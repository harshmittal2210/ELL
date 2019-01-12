////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     ComputeContext.h (value)
//  Authors:  Kern Handa
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "EmitterContext.h"
#include "FunctionDeclaration.h"
#include "Scalar.h"

#include <forward_list>
#include <map>
#include <optional>
#include <stack>
#include <string>

namespace ell
{
namespace value
{

    /// <summary> A specialization of EmitterContext designed to execute statements directly </summary>
    class ComputeContext : public EmitterContext
    {
    public:
        /// <summary> Constructor </summary>
        /// <param name="moduleName"> The name of the module that this context represents </param>
        ComputeContext(std::string moduleName);

        const ConstantData& GetConstantData(Value value) const;

    private:
        Value AllocateImpl(ValueType type, MemoryLayout layout) override;

        std::optional<Value> GetGlobalValue(GlobalAllocationScope scope, std::string name) override;

        Value GlobalAllocateImpl(GlobalAllocationScope scope, std::string name, ConstantData data, MemoryLayout layout) override;
        Value GlobalAllocateImpl(GlobalAllocationScope scope, std::string name, ValueType type, MemoryLayout layout) override;

        detail::ValueTypeDescription GetTypeImpl(Emittable emittable) override;

        DefinedFunction CreateFunctionImpl(FunctionDeclaration decl, DefinedFunction fn) override;

        Value StoreConstantDataImpl(ConstantData data) override;

        void ForImpl(MemoryLayout layout, std::function<void(std::vector<Scalar>)> fn) override;

        void MoveDataImpl(Value& source, Value& destination) override;

        void CopyDataImpl(const Value& source, Value& destination) override;

        Value OffsetImpl(Value begin, Value index) override;

        Value UnaryOperationImpl(ValueUnaryOperation op, Value destination) override;

        Value BinaryOperationImpl(ValueBinaryOperation op, Value destination, Value source) override;

        Value LogicalOperationImpl(ValueLogicalOperation op, Value source1, Value source2) override;

        Value CastImpl(Value value, ValueType type) override;

        IfContext IfImpl(Scalar test, std::function<void()> fn) override;

        std::optional<Value> CallImpl(FunctionDeclaration func, std::vector<Value> args) override;

        Value IntrinsicCall(FunctionDeclaration intrinsic, std::vector<Value> args);

        bool ValidateValue(Value value) const;
        bool TypeCompatible(Value value1, Value value2) const;

        ConstantData ExtractConstantData(Value value);
        bool IsGlobalValue(Value value);

        std::string GetScopeAdjustedName(GlobalAllocationScope scope, std::string name) const;
        std::string GetGlobalScopedName(std::string name) const;
        std::string GetCurrentFunctionScopedName(std::string name) const;

        using ConstantDataList = std::forward_list<ConstantData>;
        using Frame = std::pair<std::string, ConstantDataList>;

        Frame& GetTopFrame();
        const Frame& GetTopFrame() const;

        class IfContextImpl;
        struct FunctionScope;

        std::stack<Frame> _stack;
        std::map<std::string, std::pair<ConstantData, MemoryLayout>> _globals;
        std::unordered_map<FunctionDeclaration, DefinedFunction> _definedFunctions;
        std::string _moduleName;
    };

} // namespace value
} // namespace ell
