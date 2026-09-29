/*
 * C++ header for the __llm__ pass over one translation unit.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++  is  free  software;  you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free  Software  Foundation;  either  version 3 of the License, or (at
 * your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS  FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You  should  have  received  a copy of the GNU General Public License
 * along with llmc++; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LLMCPP_GENERATION_ENTRY_POINT_H
#define LLMCPP_GENERATION_ENTRY_POINT_H

#include "llmcpp/data/data_generation_options.h"
#include "llmcpp/data/data_generation_result.h"

#include "llvm/ADT/ArrayRef.h"
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /**
     * Run generation over one Clang frontend invocation.
     *
     * @param cc1Args Clang `-cc1` arguments without the `-cc1` marker.
     * @param opts Driver and generation options.
     * @return Pass status and rewritten source when generation succeeds.
     */
    data::DataGenerationResult run_llm_pass(llvm::ArrayRef<const char *> cc1Args,
                                            const data::DataGenerationOptions &opts);
}

#endif
