/*
 * C++ header for the __llm__ pass over one translation unit.
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
