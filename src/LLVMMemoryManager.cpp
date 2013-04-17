/*
 *    LLVMMemoryManager.cpp
 *
 *    Implementation of the MM aware of LLVM specifics
 *    such as function stack traversing.
 *
 *    LLST (LLVM Smalltalk or Lo Level Smalltalk) version 0.1
 *
 *    LLST is
 *        Copyright (C) 2012 by Dmitry Kashitsyn   aka Korvin aka Halt <korvin@deeptown.org>
 *        Copyright (C) 2012 by Roman Proskuryakov aka Humbug          <humbug@deeptown.org>
 *
 *    LLST is based on the LittleSmalltalk which is
 *        Copyright (C) 1987-2005 by Timothy A. Budd
 *        Copyright (C) 2007 by Charles R. Childers
 *        Copyright (C) 2005-2007 by Danny Reinhold
 *
 *    Original license of LittleSmalltalk may be found in the LICENSE file.
 *
 *
 *    This file is part of LLST.
 *    LLST is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    LLST is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LLST.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <memory.h>
#include <stdio.h>

// This will be used by llvm functions to store frame stack info
extern "C" { LLVMMemoryManager::TStackEntry* llvm_gc_root_chain = 0; }

void LLVMMemoryManager::moveObjects()
{
    // First of all doing our usual job
    BakerMemoryManager::moveObjects();

    // Then, traversing the call stack pointers
    for (TStackEntry* entry = llvm_gc_root_chain; entry != 0; entry = entry->next) {
        // NOTE We do not using the meta info

        // Iterating through the roots in the current stack frame
        for (int32_t index = 0, count = entry->map->numRoots; index < count; index++) {
            TMovableObject* object = (TMovableObject*) entry->roots[index];

            if (object != 0)
                object = moveObject(object);

            entry->roots[index] = object;
        }
    }
}

LLVMMemoryManager::LLVMMemoryManager()
{
}

LLVMMemoryManager::~LLVMMemoryManager()
{
    
}