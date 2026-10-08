/*
 * C++ file for shadow-compiler-backed type inspection tools.
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

// Project headers for type inspection and compiler-context formatting.
#include "llmcpp/compiler_type_inspector.h"
#include "llmcpp/compiler_ast_text.h"
#include "llmcpp/compiler_sandbox.h"
#include "llmcpp/compiler_type_probe_finder.h"
#include "llmcpp/source_text.h"

// Clang headers for AST traversal and semantic type analysis.
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Sema/Sema.h"

// LLVM headers for integer, string, JSON, and output support.
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

// Standard library header for ordered sets.
#include <set>

// Namespace import for Clang AST and semantic types.
using namespace clang;
// Type alias for lightweight LLVM string views.
using llvm::StringRef;
// Namespace alias for LLVM JSON types.
namespace json = llvm::json;

// Namespace for source-aware type inspection tools.
namespace llmcpp
{
    // Namespace for type inspection helpers private to this translation unit.
    namespace
    {

        // Locate the generated probe alias in the main file.
        const TypeAliasDecl *find_probe(ASTContext &ctx)
        {
            SourceManager &sm = ctx.getSourceManager();
            CompilerTypeProbeFinder finder;
            for (Decl *d : ctx.getTranslationUnitDecl()->decls()) {
                if (!sm.isInMainFile(sm.getExpansionLoc(d->getLocation()))) {
                    continue;
                }
                finder.TraverseDecl(d);
                if (finder.m_found) {
                    break;
                }
            }
            return finder.m_found;
        }

        // Serialize tool output as indented JSON.
        std::string pretty(json::Object o)
        {
            return llvm::formatv("{0:2}", json::Value(std::move(o))).str();
        }

        // Describe one base class and its access mode.
        std::string base_string(const CXXBaseSpecifier &base, const ASTContext &ctx)
        {
            return std::string(access_name(base.getAccessSpecifier())) +
                   (base.isVirtual() ? " virtual " : " ") + print_type(base.getType(), ctx);
        }

    }

    // Bind type tools to pass state and the active generation target.
    CompilerTypeInspector::CompilerTypeInspector(GenerationContext &state,
                                                 data::DataGenerationTarget &t)
        : m_state(state)
        , m_target(t)
    {
        // Empty.
    }

    // Resolve a type by compiling a temporary alias in shadow source.
    data::DataToolResult CompilerTypeInspector::with_probe_type(StringRef type, ProbeFn fn)
    {
        type = type.trim();
        if (type.empty() || type.find_first_of(";{}") != StringRef::npos) {
            return {"expected a type such as std::vector<int>", true};
        }

        std::string candidate = "using __llmcpp_probe_type = " + type.str() + ";";
        unsigned b = 0, e = 0;
        std::string src = m_state.shadow_source(m_target, candidate, b, e);
        std::string out;
        bool found = false;
        data::DataCompilationResult r =
            m_state.m_shadow->compile(src, b, e, false, "", [&](ASTContext &c, Sema &s) {
                if (const TypeAliasDecl *probe = find_probe(c)) {
                    found = true;
                    out = fn(c, s, probe->getUnderlyingType(), probe);
                }
            });
        if (!r.m_ok || !found) {
            return {"'" + type.str() + "' is not a valid type here:\n" + r.m_text, false};
        }
        return {out, false};
    }

    // Report semantic properties of a type visible at the target.
    data::DataToolResult CompilerTypeInspector::describe_type(StringRef type)
    {
        return with_probe_type(type, [](ASTContext &c, Sema &s, QualType ty,
                                        const TypeAliasDecl *probe) {
            json::Object o{{"type", print_type(ty, c)},
                           {"canonical", print_type(ty.getCanonicalType(), c)}};
            if (ty->isLValueReferenceType()) {
                o["reference"] = "lvalue reference";
            } else if (ty->isRValueReferenceType()) {
                o["reference"] = "rvalue reference";
            }
            QualType n = ty.getNonReferenceType();
            if (n.isConstQualified()) {
                o["const"] = true;
            }
            if (n->isDependentType()) {
                o["dependent"] = true;
                o["note"] = "the type depends on a template parameter, so its members "
                            "and properties are unknown until instantiation";
                return pretty(std::move(o));
            }

            const char *kind = n->isBuiltinType()    ? "builtin"
                               : n->isPointerType()  ? "pointer"
                               : n->isArrayType()    ? "array"
                               : n->isEnumeralType() ? "enum"
                               : n->isUnionType()    ? "union"
                               : n->isRecordType()   ? "class"
                               : n->isFunctionType() ? "function"
                                                     : n->getTypeClassName();
            o["kind"] = kind;
            if (n->isPointerType()) {
                o["pointee"] = print_type(n->getPointeeType(), c);
            }

            if (!n->isFunctionType() && !n->isVoidType()) {
                bool complete = s.isCompleteType(probe->getLocation(), n);
                o["complete"] = complete;
                if (complete) {
                    o["size_bytes"] = c.getTypeSizeInChars(n).getQuantity();
                    o["align_bytes"] = c.getTypeAlignInChars(n).getQuantity();
                    o["trivially_copyable"] = n.isTriviallyCopyableType(c);
                }
            }

            if (CXXRecordDecl *rd = n->getAsCXXRecordDecl(); rd && rd->hasDefinition()) {
                rd = rd->getDefinition();
                o["declared_at"] = location_string(rd->getLocation(), c.getSourceManager());
                o["polymorphic"] = rd->isPolymorphic();
                o["abstract"] = rd->isAbstract();
                o["aggregate"] = rd->isAggregate();
                auto usable = [](const CXXConstructorDecl *ctor) {
                    return ctor && !ctor->isDeleted();
                };
                o["default_constructible"] = usable(s.LookupDefaultConstructor(rd));
                o["copy_constructible"] = usable(s.LookupCopyingConstructor(rd, Qualifiers::Const));
                o["move_constructible"] = usable(s.LookupMovingConstructor(rd, 0));

                json::Array bases;
                for (const CXXBaseSpecifier &base : rd->bases()) {
                    bases.push_back(base_string(base, c));
                }
                if (!bases.empty()) {
                    o["bases"] = std::move(bases);
                }

                if (const auto *spec = dyn_cast<ClassTemplateSpecializationDecl>(rd)) {
                    json::Array args;
                    for (const TemplateArgument &a : spec->getTemplateArgs().asArray()) {
                        std::string s2;
                        llvm::raw_string_ostream os(s2);
                        a.print(c.getPrintingPolicy(), os, true);
                        args.push_back(s2);
                    }
                    o["template"] = spec->getSpecializedTemplate()->getQualifiedNameAsString();
                    o["template_arguments"] = std::move(args);
                }

                std::set<std::string> ops;
                for (const Decl *d : rd->decls()) {
                    const FunctionDecl *fd = d->getAsFunction();
                    if (fd && fd->isOverloadedOperator() && !fd->isImplicit() && !fd->isDeleted()) {
                        ops.insert(fd->getNameAsString());
                    }
                }
                if (!ops.empty()) {
                    o["member_operators"] = llvm::join(ops, ", ");
                }
                o["hint"] = "list_members shows fields and methods; operators declared "
                            "outside the class (such as operator<<) can be found with "
                            "lookup";
            }

            if (const auto *et = n->getAs<EnumType>()) {
                const EnumDecl *ed = et->getDecl();
                o["scoped"] = ed->isScoped();
                o["underlying_type"] = print_type(ed->getIntegerType(), c);
                json::Array enumerators;
                unsigned count = 0;
                for (const EnumConstantDecl *ec : ed->enumerators()) {
                    if (++count > 100) {
                        break;
                    }
                    enumerators.push_back(ec->getName().str() + " = " +
                                          llvm::toString(ec->getInitVal(), 10));
                }
                o["enumerators"] = std::move(enumerators);
            }
            return pretty(std::move(o));
        });
    }

    // List accessible members of a class type visible at the target.
    data::DataToolResult CompilerTypeInspector::list_members(StringRef type)
    {
        return with_probe_type(
            type,
            [](ASTContext &c, Sema &s, QualType ty, const TypeAliasDecl *probe) -> std::string {
                std::string out;
                llvm::raw_string_ostream os(out);
                QualType n = ty.getNonReferenceType();
                if (n->isPointerType()) {
                    os << "(members of the pointee type " << print_type(n->getPointeeType(), c)
                       << ")\n";
                    n = n->getPointeeType();
                }
                CXXRecordDecl *rd = n->getAsCXXRecordDecl();
                if (!rd) {
                    if (n->isDependentType()) {
                        return "the type depends on a template parameter; its members are "
                               "unknown until instantiation";
                    }
                    return "'" + print_type(n, c) + "' is not a class type and has no members";
                }
                if (!s.isCompleteType(probe->getLocation(), n) || !rd->hasDefinition()) {
                    return "'" + print_type(n, c) + "' is an incomplete type here";
                }
                rd = rd->getDefinition();
                const DeclContext *from = probe->getDeclContext();

                std::vector<std::string> bases, ctors, methods, fields, statics, types, other;
                unsigned hidden = 0;
                for (const CXXBaseSpecifier &base : rd->bases()) {
                    bases.push_back(base_string(base, c));
                }
                for (const Decl *d : rd->decls()) {
                    const auto *nd = dyn_cast<NamedDecl>(d);
                    if (!nd || d->isImplicit() || isa<IndirectFieldDecl>(nd) ||
                        isa<UsingShadowDecl>(nd)) {
                        continue;
                    }
                    if (!accessible_from(nd, from)) {
                        ++hidden;
                        continue;
                    }
                    std::string line =
                        std::string(access_name(nd->getAccess())) + ": " + print_decl(nd, c);
                    const Decl *inner = nd;
                    if (const auto *ft = dyn_cast<FunctionTemplateDecl>(nd)) {
                        inner = ft->getTemplatedDecl();
                    }
                    if (isa<CXXDestructorDecl>(inner)) {
                        continue;
                    }
                    if (isa<CXXConstructorDecl>(inner)) {
                        ctors.push_back(line);
                    } else if (isa<CXXMethodDecl>(inner)) {
                        methods.push_back(line);
                    } else if (isa<FieldDecl>(nd)) {
                        fields.push_back(line);
                    } else if (isa<VarDecl>(nd) || isa<VarTemplateDecl>(nd)) {
                        statics.push_back(line);
                    } else if (isa<TypeDecl>(nd) || isa<ClassTemplateDecl>(nd) ||
                               isa<TypeAliasTemplateDecl>(nd)) {
                        types.push_back(line);
                    } else {
                        other.push_back(line);
                    }
                }

                os << kind_name(rd) << " " << print_type(c.getRecordType(rd), c) << "\n";
                unsigned printed = 0;
                bool truncated = false;
                auto section = [&](StringRef title, const std::vector<std::string> &lines) {
                    if (lines.empty() || truncated) {
                        return;
                    }
                    os << title << ":\n";
                    for (const std::string &l : lines) {
                        if (printed++ == 250) {
                            truncated = true;
                            return;
                        }
                        os << "  " << l << "\n";
                    }
                };
                section("bases", bases);
                section("constructors", ctors);
                section("methods", methods);
                section("fields", fields);
                section("static members", statics);
                section("member types", types);
                section("other", other);
                if (truncated) {
                    os << "... (truncated; use lookup with a qualified name for a specific "
                          "member)\n";
                }
                if (hidden) {
                    os << hidden
                       << " private or protected members that aren't accessible here are "
                          "not shown.\n";
                }
                if (!bases.empty()) {
                    os << "Inherited members are not listed; call list_members on a base.\n";
                }
                return out;
            });
    }

}
