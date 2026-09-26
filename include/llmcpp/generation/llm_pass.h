/*
 * C++ header for the __llm__ pass over one translation unit.
 */

#ifndef LLMCPP_LLMPASS_H
#define LLMCPP_LLMPASS_H

#include "llmcpp/data/options.h"
#include "llmcpp/data/pass_result.h"

#include "llvm/ADT/ArrayRef.h"
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp generation declarations. */
    namespace generation
    {
        /**
         * Run generation over one Clang frontend invocation.
         *
         * @param cc1Args Clang `-cc1` arguments without the `-cc1` marker.
         * @param opts Driver and generation options.
         * @return Pass status and rewritten source when generation succeeds.
         */
        data::PassResult run_llm_pass(llvm::ArrayRef<const char *> cc1Args,
                                      const data::Options &opts);
    }
}

#endif
