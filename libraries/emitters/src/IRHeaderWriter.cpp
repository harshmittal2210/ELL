////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     IRHeaderWriter.h (emitters)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "IRHeaderWriter.h"
#include "EmitterException.h"

// llvm
#include "llvm/IR/Attributes.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_os_ostream.h"

// stl
#include <string>

namespace ell
{
namespace emitters
{
    namespace
    {
        void WriteLLVMType(std::ostream& os, llvm::Type* t);

        // This heuristic for deciding if a type should be declared in the header file is fragile and temporary.
        bool ShouldWriteType(const llvm::StructType* t)
        {
            if (t->hasName()) // && !t->isLiteral() ?
            {
                std::string typeName = t->getName();
                {
                    if (typeName == "timespec")
                    {
                        return false;
                    }
                    if (typeName.find("struct.") == 0 || typeName.find("class.") == 0)
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        // This heuristic for deciding if a function should be declared in the header file is fragile and temporary.
        bool ShouldWriteFunction(const llvm::Function& f)
        {
            auto hasName = f.hasName();
            if (!hasName)
            {
                return false;
            }

            std::string functionName = f.getName();
            if (functionName.find("_Node__") == 0)
            {
                return false;
            }
            if (functionName.find("llvm.") == 0)
            {
                return false;
            }
            if (functionName.find("clock_gettime") == 0)
            {
                return false;
            }
            if (functionName.find("cblas") == 0)
            {
                return false;
            }
            if (functionName.find("printf") == 0)
            {
                return false;
            }
            if (functionName.find("_") == 0)
            {
                return false;
            }
            return true;
        }

        void WriteStructType(std::ostream& os, llvm::StructType* t)
        {
            if (t->hasName()) // && !t->isLiteral() ?
            {
                std::string typeName = t->getName();
                os << "struct " << typeName;
            }
        }

        void WriteArrayType(std::ostream& os, llvm::ArrayType* t)
        {
            auto size = t->getNumElements();
            auto elemType = t->getTypeAtIndex(0u);
            WriteLLVMType(os, elemType);
            os << "[" << size << "]";
        }

        void WritePointerType(std::ostream& os, llvm::PointerType* t)
        {
            auto elemType = t->getTypeAtIndex(0u);
            WriteLLVMType(os, elemType);
            os << "*";
        }

        void WriteIntegerType(std::ostream& os, llvm::IntegerType* t)
        {
            auto size = t->getBitWidth();
            os << "int" << size << "_t";
        }

        void WriteFunctionType(std::ostream& os, llvm::FunctionType* t)
        {
            auto returnType = t->getReturnType();
            WriteLLVMType(os, returnType);
            os << " (";
            bool first = true;
            for (auto pt : t->params())
            {
                if (!first)
                {
                    os << ", ";
                }
                first = false;
                WriteLLVMType(os, pt);
            }
            os << ");";
        }

        void WriteLLVMType(std::ostream& os, llvm::Type* t)
        {
            if (t->isStructTy())
            {
                WriteStructType(os, llvm::cast<llvm::StructType>(t));
            }
            else if (t->isArrayTy())
            {
                WriteArrayType(os, llvm::cast<llvm::ArrayType>(t));
            }
            else if (t->isPointerTy())
            {
                WritePointerType(os, llvm::cast<llvm::PointerType>(t));
            }
            else if (t->isIntegerTy())
            {
                WriteIntegerType(os, llvm::cast<llvm::IntegerType>(t));
            }
            else if (t->isFloatTy())
            {
                os << "float";
            }
            else if (t->isDoubleTy())
            {
                os << "double";
            }
            else if (t->isVoidTy())
            {
                os << "void";
            }
            else if (t->isFunctionTy())
            {
                WriteFunctionType(os, llvm::cast<llvm::FunctionType>(t));
            }
            else
            {
                os << "[[UNKNOWN]]";
                // look up in table
                // ???
            }
        }

        void WriteLLVMVarDecl(std::ostream& os, llvm::Type* t, std::string name)
        {
            if (t->isArrayTy())
            {
                auto arrType = llvm::cast<llvm::ArrayType>(t);
                auto size = arrType->getNumElements();
                auto elemType = arrType->getTypeAtIndex(0u);
                WriteLLVMType(os, elemType);
                os << " " << name << "[" << size << "]";
            }
            else
            {
                WriteLLVMType(os, t);
                os << " " << name;
            }
        }

        void WriteStructDefinition(std::ostream& os, llvm::StructType* t)
        {
            if (t->hasName()) // && !t->isLiteral() ?
            {
                std::string typeName = t->getName();
                os << "struct " << typeName << "\n";
                os << "{\n";
                auto index = 0;
                for (auto& fieldType : t->elements())
                {
                    os << "    ";
                    std::string fieldName = std::string("param") + std::to_string(index);
                    WriteLLVMVarDecl(os, fieldType, fieldName);
                    os << ";\n";
                    ++index;
                }
                os << "};";
            }
        }

        void WriteFunction(std::ostream& os, IRModuleEmitter& moduleEmitter, llvm::Function& f)
        {
            auto hasName = f.hasName();
            if (hasName)
            {
                std::string name = f.getName();

                // Check if we've added comments for this function
                if (moduleEmitter.HasFunctionComments(name))
                {
                    auto comments = moduleEmitter.GetFunctionComments(name);
                    for (auto comment : comments)
                    {
                        os << "// " << comment << "\n";
                    }
                }

                // Now write the function signature
                auto returnType = f.getReturnType();
                WriteLLVMType(os, returnType);
                os << " " << name << "(";
                bool first = true;
                for (const auto& arg : f.args())
                {
                    if (!first)
                    {
                        os << ", ";
                    }
                    first = false;
                    WriteLLVMType(os, arg.getType());

                    bool hasParamName = false;
                    if (hasParamName)
                    {
                        auto paramName = "param";
                        os << " " << paramName;
                    }
                }
                os << ");";
            }
        }
    }

    void WriteModuleHeader(std::ostream& os, IRModuleEmitter& moduleEmitter)
    {
        auto pModule = moduleEmitter.GetLLVMModule();

        // Header comment
        std::string moduleName = pModule->getName();
        os << "//\n// ELL header for module " << moduleName << "\n//\n\n";

        os << "#include <stdint.h>\n\n";
        os << "#ifdef __cplusplus\n";
        os << "extern \"C\"{\n";
        os << "#endif\n";

        // preprocessor definitions
        auto defines = moduleEmitter.GetPreprocessorDefinitions();
        if (defines.size() > 0)
        {
            for (const auto& def : defines)
            {
                os << "#define " << def.first << " " << def.second << "\n";
            }
            os << "\n";
        }

        // First write out type definitions
        os << "//\n// Types\n//\n\n";
        auto structTypes = pModule->getIdentifiedStructTypes();
        for (auto t : structTypes)
        {
            if (ShouldWriteType(t))
            {
                WriteStructDefinition(os, t);
                os << "\n\n";
            }
        }

        os << "\n";
        os << "//\n// Functions\n//\n\n";
        // Now write out function signatures
        for (auto& f : pModule->functions())
        {
            if (ShouldWriteFunction(f))
            {
                WriteFunction(os, moduleEmitter, f);
                os << "\n\n";
            }
        }

        os << "#ifdef __cplusplus\n";
        os << "} // extern \"C\"\n";
        os << "#endif\n";
    }
}
}
