#ifndef NATIVE_METHOD_POINTER_H_INCLUDED
#define NATIVE_METHOD_POINTER_H_INCLUDED

#include <cstddef>
#include <stdexcept>
#include <types.h>
#include <type_traits>
#include <utility>

#include <api.h>
#include <types.h>

// we might want to rename it to marshalling error
// or type mismatch error
class NativeMethodInvocationError : public std::runtime_error {
public:
    template <typename... Args>
    NativeMethodInvocationError(Args&&... args) :
        std::runtime_error(std::forward<Args>(args)...) { }
};

class SmalltalkVM;

class TNativeMethodBase {
public:
    virtual TObject* operator ()(SmalltalkVM* vm, const TObjectArray* args) const = 0;

    // you can additional methods for introspection, e.g.
    // virtual size_t argumentsCount() const = 0;
    // virtual std::vector<TClass*> argumentTypes() const = 0;

    virtual ~TNativeMethodBase() { }
};

namespace impl {

// generates sequences of numbers
// gen<N>::type = seq<0, 1, 2, ..., N-1>

template<size_t ...S>
struct seq {
    template <size_t N>
    struct add {
        typedef seq<S + N...> type;
    };
};

template <size_t N, size_t... S>
inline size_t max(seq<N, S...>) { return std::max(N, max(seq<S...>())); }

inline size_t max(seq<>) { return 0; }

template<size_t N, size_t ...S>
struct gens : gens<N-1, N-1, S...> { };

template<size_t ...S>
struct gens<0, S...> {
      typedef seq<S...> type;
};


template <typename VM, typename Arg, size_t Index>
struct ArgumentExtractor { };

// convert to pointer
template <typename VM, typename T, size_t Index>
struct ArgumentExtractor<VM, T*, Index> {
    T* operator()(VM* vm, const TObjectArray* args) const {
        try {
            return vm->template checked_cast<T*>(args->getField(Index));
        } catch (const std::bad_cast& exc) {
            throw NativeMethodInvocationError(exc.what());
        }
    }
};

// convert to small integer
template <typename VM, size_t Index>
struct ArgumentExtractor<VM, TInteger, Index> {
    TInteger operator()(VM*, const TObjectArray* args) const { 
        try {
            return args->getField(Index);
        } catch (const std::bad_cast& exc) {
            throw NativeMethodInvocationError("SmallInteger expected");
        }
    }
};

// "extract" VM argument
template <typename VM>
struct ArgumentExtractor<VM, VM*, 0> {
    VM* operator ()(VM* vm, const TObjectArray* /*args*/) const { return vm; }
};

template <typename R, typename Enable = void>
struct Invoker { };

// convert result from pointer to derived class or from small integer (implicitly convertible)
template <typename R>
struct Invoker<R, typename std::enable_if<std::is_convertible<R, TObject*>::value>::type> {
    template <typename T, typename... Args>
    TObject* operator ()(T* self, R (T::*method)(Args...), Args... args) {
        return (self->*method)(args...);
    }
};

// return nilObject if the method returns void
template <>
struct Invoker<void> {
    template <typename T, typename... Args>
    TObject* operator ()(T* self, void (T::*method)(Args...), Args... args) {
        (self->*method)(args...);
        return globals.nilObject;
    }
};

template <typename VM, typename... Args>
struct VMArgumentAnalyzer {
    typedef typename gens<sizeof...(Args)>::type::template add<1>::type Indexes;
};

template <typename VM, typename... Args>
struct VMArgumentAnalyzer<VM, VM*, Args...> {
    // the first argument is special: VM*
    typedef typename gens<sizeof...(Args) + 1>::type Indexes;
};

} // namespace impl

template <typename VM, typename T, typename R, typename... Args>
class TNativeMethodPointer : public TNativeMethodBase {
public:
    typedef R (T::*Fptr)(Args... args);

    TNativeMethodPointer(Fptr method) : m_method(method) { }

    virtual TObject* operator ()(VM* vm, const TObjectArray* args) const {
        typedef typename impl::VMArgumentAnalyzer<VM, Args...>::Indexes Indexes;

        if (args->getSize() != max(Indexes()) + 1) {
            throw NativeMethodInvocationError(
                    "Incorrect number of arguments passed to native method");
        }

        return call(vm, args, Indexes());
    }

private:
    template <size_t... Indexes>
    TObject* call(VM* vm, const TObjectArray* args, impl::seq<Indexes...>) const {
        T* self;

        try {
            self = vm->template checked_cast<T*>(args->getField(0));
        } catch (const std::bad_cast& exc) {
            throw NativeMethodInvocationError(exc.what());
        }

        return impl::Invoker<R>()(self, m_method, impl::ArgumentExtractor<VM, Args, Indexes>()(vm, args)...);
    }

    Fptr m_method;
};

#endif
