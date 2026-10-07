/*
 * C++ file for formatting helpers for Clang AST entities.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++ is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with llmc++; if not, see
 * <https://www.gnu.org/licenses/>.
 */

// Project header for AST text declarations.
#include "llmcpp/compiler_ast_text.h"
#include "llmcpp/source_text.h"

// Clang headers for AST declarations, printing, and source locations.
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/Basic/SourceManager.h"

// LLVM headers for filesystem paths and output streams.
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

// Namespace import for Clang AST types.
using namespace clang;
// Type alias for lightweight LLVM string views.
using llvm::StringRef;

// Namespace for Clang AST formatting helpers.
namespace llmcpp
{
    // Namespace for AST text implementation details private to this translation unit.
    namespace
    {

        // Create a concise AST declaration printing policy.
        PrintingPolicy decl_policy(const ASTContext &ctx)
        {
            PrintingPolicy policy = ctx.getPrintingPolicy();
            policy.TerseOutput = true;
            policy.PolishForDeclaration = true;
            policy.SuppressUnwrittenScope = true;
            return policy;
        }

        // Format the parameter list of a function declaration.
        std::string parameter_list(const FunctionDecl *function, const ASTContext &ctx)
        {
            std::string text;
            for (unsigned i = 0; i < function->getNumParams(); ++i) {
                const ParmVarDecl *parameter = function->getParamDecl(i);
                if (i) {
                    text += ", ";
                }
                text += print_type(parameter->getType(), ctx);
                if (!parameter->getName().empty()) {
                    text += (text.back() == '&' || text.back() == '*' ? "" : " ") +
                            parameter->getName().str();
                }
            }
            if (function->isVariadic()) {
                text += function->getNumParams() ? ", ..." : "...";
            }
            return text;
        }

    }

    // Format a type for agent-facing output.
    std::string print_type(QualType type, const ASTContext &ctx)
    {
        PrintingPolicy policy = ctx.getPrintingPolicy();
        policy.SuppressUnwrittenScope = true;
        return type.getAsString(policy);
    }

    // Format a declaration for agent-facing output.
    std::string print_decl(const Decl *decl, const ASTContext &ctx)
    {
        if (const auto *ns = dyn_cast<NamespaceDecl>(decl)) {
            return "namespace " + ns->getQualifiedNameAsString();
        }
        std::string text;
        llvm::raw_string_ostream output(text);
        decl->print(output, decl_policy(ctx));
        std::string result = collapse_whitespace(text);
        if (result.size() > 400) {
            result = result.substr(0, 400) + " ...";
        }
        return result;
    }

    // Format a compact source location.
    std::string location_string(SourceLocation location, const SourceManager &sourceManager)
    {
        if (location.isInvalid()) {
            return "";
        }
        PresumedLoc presumed =
            sourceManager.getPresumedLoc(sourceManager.getExpansionLoc(location));
        if (presumed.isInvalid()) {
            return "";
        }
        return (llvm::sys::path::filename(presumed.getFilename()) + ":" +
                llvm::Twine(presumed.getLine()))
            .str();
    }

    // Format a function signature with relevant semantic qualifiers.
    std::string function_signature(const FunctionDecl *function, const ASTContext &ctx)
    {
        std::string text;
        if (const FunctionTemplateDecl *functionTemplate =
                function->getDescribedFunctionTemplate()) {
            text += "template <";
            const TemplateParameterList *parameters = functionTemplate->getTemplateParameters();
            for (unsigned i = 0; i < parameters->size(); ++i) {
                text += (i ? ", " : "") + print_decl(parameters->getParam(i), ctx);
            }
            text += "> ";
        }
        const auto *method = dyn_cast<CXXMethodDecl>(function);
        if (method && method->isStatic()) {
            text += "static ";
        }
        if (method && method->isVirtual()) {
            text += "virtual ";
        }
        if (!isa<CXXConstructorDecl>(function) && !isa<CXXDestructorDecl>(function)) {
            text += print_type(function->getDeclaredReturnType(), ctx) + " ";
        }
        text += function->getQualifiedNameAsString() + "(" + parameter_list(function, ctx) + ")";
        if (method) {
            if (method->isConst()) {
                text += " const";
            }
            if (method->isVolatile()) {
                text += " volatile";
            }
            if (method->getRefQualifier() == RQ_LValue) {
                text += " &";
            } else if (method->getRefQualifier() == RQ_RValue) {
                text += " &&";
            }
        }
        if (const auto *functionType = function->getType()->getAs<FunctionProtoType>()) {
            if (functionType->hasNoexceptExceptionSpec() && functionType->isNothrow()) {
                text += " noexcept";
            }
        }
        return text;
    }

    // Format a lambda signature and its explicit captures.
    std::string lambda_signature(const LambdaExpr *lambda, const ASTContext &ctx)
    {
        std::string text = "[";
        bool first = true;
        auto addCapture = [&](const std::string &part) {
            text += (first ? "" : ", ") + part;
            first = false;
        };
        if (lambda->getCaptureDefault() == LCD_ByCopy) {
            addCapture("=");
        } else if (lambda->getCaptureDefault() == LCD_ByRef) {
            addCapture("&");
        }
        for (const LambdaCapture &capture : lambda->explicit_captures()) {
            switch (capture.getCaptureKind()) {
                case LCK_This:
                    addCapture("this");
                    break;
                case LCK_StarThis:
                    addCapture("*this");
                    break;
                case LCK_ByRef:
                    addCapture("&" + capture.getCapturedVar()->getName().str());
                    break;
                case LCK_ByCopy:
                    addCapture(capture.getCapturedVar()->getName().str());
                    break;
                case LCK_VLAType:
                    break;
            }
        }
        const CXXMethodDecl *call = lambda->getCallOperator();
        text += "](" + parameter_list(call, ctx) + ")";
        if (lambda->isMutable()) {
            text += " mutable";
        }
        if (lambda->hasExplicitResultType()) {
            text += " -> " + print_type(call->getDeclaredReturnType(), ctx);
        }
        return text;
    }

    // Return a human-readable declaration kind.
    std::string kind_name(const NamedDecl *decl)
    {
        if (isa<NamespaceDecl>(decl) || isa<NamespaceAliasDecl>(decl)) {
            return "namespace";
        }
        if (isa<CXXConstructorDecl>(decl)) {
            return "constructor";
        }
        if (isa<CXXMethodDecl>(decl)) {
            return "method";
        }
        if (isa<FunctionDecl>(decl)) {
            return "function";
        }
        if (isa<FunctionTemplateDecl>(decl)) {
            return "function template";
        }
        if (isa<ClassTemplateDecl>(decl)) {
            return "class template";
        }
        if (isa<TypeAliasTemplateDecl>(decl)) {
            return "alias template";
        }
        if (isa<VarTemplateDecl>(decl)) {
            return "variable template";
        }
        if (isa<ConceptDecl>(decl)) {
            return "concept";
        }
        if (const auto *record = dyn_cast<RecordDecl>(decl)) {
            return record->isUnion() ? "union" : record->isStruct() ? "struct" : "class";
        }
        if (isa<EnumDecl>(decl)) {
            return "enum";
        }
        if (isa<EnumConstantDecl>(decl)) {
            return "enumerator";
        }
        if (isa<TypedefNameDecl>(decl)) {
            return "type alias";
        }
        if (isa<FieldDecl>(decl)) {
            return "field";
        }
        if (isa<ParmVarDecl>(decl)) {
            return "parameter";
        }
        if (const auto *variable = dyn_cast<VarDecl>(decl)) {
            return variable->isLocalVarDecl()       ? "local variable"
                   : variable->isStaticDataMember() ? "static data member"
                                                    : "variable";
        }
        return decl->getDeclKindName();
    }

    // Return a human-readable C++ access level.
    const char *access_name(AccessSpecifier access)
    {
        switch (access) {
            case AS_public:
                return "public";
            case AS_protected:
                return "protected";
            case AS_private:
                return "private";
            case AS_none:
                return "none";
        }
        return "none";
    }

    // Approximate whether a declaration is accessible from a context.
    bool accessible_from(const NamedDecl *decl, const DeclContext *from)
    {
        AccessSpecifier access = decl->getAccess();
        if (access == AS_public || access == AS_none) {
            return true;
        }
        const auto *owner = dyn_cast<CXXRecordDecl>(decl->getDeclContext());
        if (!owner) {
            return true;
        }
        for (const DeclContext *context = from; context; context = context->getParent()) {
            if (const auto *record = dyn_cast<CXXRecordDecl>(context)) {
                if (record->getCanonicalDecl() == owner->getCanonicalDecl()) {
                    return true;
                }
                if (access == AS_protected && record->hasDefinition() &&
                    record->isDerivedFrom(owner)) {
                    return true;
                }
            }
        }
        return false;
    }
}
