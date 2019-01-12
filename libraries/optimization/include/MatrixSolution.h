////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     MatrixSolution.h (optimization)
//  Authors:  Ofer Dekel
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Expression.h"
#include "IndexedContainer.h"
#include "OptimizationExample.h"

#include <math/include/Matrix.h>
#include <math/include/Vector.h>
#include <math/include/VectorOperations.h>

namespace ell
{
namespace optimization
{
    /// <summary> A matrix solution that applies to vector inputs and vector outputs. </summary>
    template <typename IOElementType, bool isBiased = false>
    class MatrixSolution : public Scalable
    {
    public:
        using InputType = math::ConstRowVectorReference<IOElementType>;
        using OutputType = math::ConstRowVectorReference<IOElementType>;
        using AuxiliaryDoubleType = math::RowVector<double>;
        using ExampleType = Example<InputType, OutputType>;
        using DatasetType = IndexedContainer<ExampleType>;

        /// <summary> Resize the solution to match the sizes of an input and an output. </summary>
        void Resize(const InputType& inputExample, const OutputType& outputExample);

        /// <summary> Resets the solution to zero. </summary>
        void Reset();

        /// <summary> Returns the matrix. </summary>
        math::ConstColumnMatrixReference<double> GetMatrix() const { return _weights; }

        /// <summary> Returns the matrix. </summary>
        math::ColumnMatrixReference<double> GetMatrix() { return _weights; }

        /// <summary> Returns a vector reference to the matrix. </summary>
        math::ConstColumnVectorReference<double> GetVector() const { return _weights.ReferenceAsVector(); }

        /// <summary> Returns a vector reference to the matrix. </summary>
        math::ColumnVectorReference<double> GetVector() { return _weights.ReferenceAsVector(); }

        /// <summary> Returns the bias. </summary>
        template <bool B = isBiased, typename Concept = std::enable_if_t<B>>
        const math::RowVector<double>& GetBias() const
        {
            return _bias;
        }

        /// <summary> Assignment operator. </summary>
        void operator=(const MatrixSolution<IOElementType, isBiased>& other);

        /// <summary> Adds another scaled solution to a scaled version of this solution. </summary>
        void operator=(SumExpression<ScaledExpression<MatrixSolution<IOElementType, isBiased>>, ScaledExpression<MatrixSolution<IOElementType, isBiased>>> expression);

        /// <summary> Adds a scaled column vector to a scaled version of this solution. </summary>
        void operator=(SumExpression<ScaledExpression<MatrixSolution<IOElementType, isBiased>>, OuterProductExpression<IOElementType>> expression);

        /// <summary> Subtracts another solution from this one. </summary>
        void operator-=(const MatrixSolution<IOElementType, isBiased>& other);

        /// <summary> Adds a scaled column vector to this solution. </summary>
        void operator+=(OuterProductExpression<IOElementType> expression);

        /// <summary> Computes input * weights, or input * weights + bias (if a bias exists). </summary>
        math::RowVector<double> Multiply(const InputType& input) const;

        /// <summary> Returns the squared 2-norm of a given input. </summary>
        static double GetNorm2SquaredOf(const InputType& input);

        /// <summary> Initializes an auxiliary double variable. </summary>
        void InitializeAuxiliaryVariable(AuxiliaryDoubleType& aux);

    private:
        math::ColumnMatrix<double> _weights = { 0, 0 };

        struct Nothing
        {};

        // if the solution is biased, allocate a bias term
        std::conditional_t<isBiased, math::RowVector<double>, Nothing> _bias = {};

        // if the IO element type is not double, allocate a double row vector
        static constexpr bool isDouble = std::is_same_v<IOElementType, double>;
        mutable std::conditional_t<isDouble, Nothing, math::RowVector<double>> _doubleInput;
    };

    /// <summary> Returns the squared 2-norm of a MatrixSolutionBase. </summary>
    template <typename IOElementType, bool isBiased>
    double Norm2Squared(const MatrixSolution<IOElementType, isBiased>& solution);

    /// <summary> vector-solution product. </summary>
    template <typename IOElementType, bool isBiased>
    math::RowVector<double> operator*(math::ConstRowVectorReference<IOElementType> input, const MatrixSolution<IOElementType, isBiased>& solution);

    /// <summary> An unbiased matrix solution that applies to vector inputs and vector outputs. </summary>
    template <typename IOElementType>
    using UnbiasedMatrixSolution = MatrixSolution<IOElementType, false>;

    /// <summary> A biased matrix solution that applies to vector inputs and vector outputs. </summary>
    template <typename IOElementType>
    using BiasedMatrixSolution = MatrixSolution<IOElementType, true>;
} // namespace optimization
} // namespace ell

#pragma region implementation

#include "Common.h"

#include <math/include/MatrixOperations.h>
#include <math/include/VectorOperations.h>

namespace ell
{
namespace optimization
{
    template <typename IOElementType, bool isBiased>
    void MatrixSolution<IOElementType, isBiased>::Resize(const InputType& inputExample, const OutputType& outputExample)
    {
        math::ColumnMatrix<double> matrix(inputExample.Size(), outputExample.Size());
        _weights.Swap(matrix);

        if constexpr (!isDouble)
        {
            _doubleInput.Resize(inputExample.Size());
        }

        if constexpr (isBiased)
        {
            _bias.Resize(outputExample.Size());
        }
    }

    template <typename IOElementType, bool isBiased>
    void MatrixSolution<IOElementType, isBiased>::Reset()
    {
        _weights.Reset();

        if constexpr (isBiased)
        {
            _bias.Reset();
        }
    }

    template <typename IOElementType, bool isBiased>
    void MatrixSolution<IOElementType, isBiased>::operator=(const MatrixSolution<IOElementType, isBiased>& other)
    {
        _weights.CopyFrom(other._weights);

        if constexpr (isBiased)
        {
            _bias.CopyFrom(other._bias);
        }
    }

    template <typename IOElementType, bool isBiased>
    void MatrixSolution<IOElementType, isBiased>::operator=(SumExpression<ScaledExpression<MatrixSolution<IOElementType, isBiased>>, ScaledExpression<MatrixSolution<IOElementType, isBiased>>> expression)
    {
        const auto& thisTerm = expression.lhs;
        const auto& otherTerm = expression.rhs;

        if (&(thisTerm.lhs.get()) != this)
        {
            throw OptimizationException("First term should be a scaled version of this solution");
        }

        double thisScale = thisTerm.rhs;
        const auto& otherSolution = otherTerm.lhs.get();
        double otherScale = otherTerm.rhs;
        math::ScaleAddUpdate(otherScale, otherSolution._weights, thisScale, _weights);

        if constexpr (isBiased)
        {
            math::ScaleAddUpdate(otherScale, otherSolution.GetBias(), thisScale, _bias);
        }
    }

    template <typename IOElementType, bool isBiased>
    void MatrixSolution<IOElementType, isBiased>::operator=(SumExpression<ScaledExpression<MatrixSolution<IOElementType, isBiased>>, OuterProductExpression<IOElementType>> expression)
    {
        const auto& thisTerm = expression.lhs;
        const auto& updateTerm = expression.rhs;

        if (&(thisTerm.lhs.get()) != this)
        {
            throw OptimizationException("The first term should be a scaled version of this solution");
        }

        double thisScale = thisTerm.rhs;
        const auto& columnVectorReference = updateTerm.lhs;
        const auto& rowVectorReference = updateTerm.rhs;
        _weights *= thisScale;

        if constexpr (isDouble)
        {
            math::RankOneUpdate(1.0, columnVectorReference, rowVectorReference, _weights);
        }
        else
        {
            auto doubleColumnVector = _doubleInput.Transpose();
            doubleColumnVector.CopyFrom(columnVectorReference);
            math::RankOneUpdate(1.0, doubleColumnVector, rowVectorReference, _weights);
        }

        if constexpr (isBiased)
        {
            math::ScaleAddUpdate(1.0, rowVectorReference, thisScale, _bias);
        }
    }

    template <typename IOElementType, bool isBiased>
    void MatrixSolution<IOElementType, isBiased>::operator-=(const MatrixSolution<IOElementType, isBiased>& other)
    {
        _weights -= other._weights;
        if constexpr (isBiased)
        {
            _bias -= other._bias;
        }
    }

    template <typename IOElementType, bool isBiased>
    void MatrixSolution<IOElementType, isBiased>::operator+=(OuterProductExpression<IOElementType> expression)
    {
        const auto& columnVectorReference = expression.lhs;
        const auto& rowVectorReference = expression.rhs;

        if constexpr (isDouble)
        {
            math::RankOneUpdate(1.0, columnVectorReference, rowVectorReference, _weights);
        }
        else
        {
            auto doubleColumnVector = _doubleInput.Transpose();
            doubleColumnVector.CopyFrom(columnVectorReference);
            math::RankOneUpdate(1.0, doubleColumnVector, rowVectorReference, _weights);
        }

        if constexpr (isBiased)
        {
            math::ScaleAddUpdate(1.0, rowVectorReference, 1.0, _bias);
        }
    }

    template <typename IOElementType, bool isBiased>
    math::RowVector<double> MatrixSolution<IOElementType, isBiased>::Multiply(const InputType& input) const
    {
        math::RowVector<double> result(_weights.NumColumns());

        if constexpr (isBiased)
        {
            result.CopyFrom(_bias);
        }

        if constexpr (isDouble)
        {
            math::MultiplyScaleAddUpdate(1.0, input, _weights, 1.0, result);
        }
        else
        {
            _doubleInput.CopyFrom(input);
            math::MultiplyScaleAddUpdate(1.0, _doubleInput, _weights, 1.0, result);
        }

        return result;
    }

    template <typename IOElementType, bool isBiased>
    double MatrixSolution<IOElementType, isBiased>::GetNorm2SquaredOf(const InputType& input)
    {
        double result = input.Norm2Squared();

        if constexpr (isBiased)
        {
            result += 1.0;
        }

        return result;
    }

    template <typename IOElementType, bool isBiased>
    void MatrixSolution<IOElementType, isBiased>::InitializeAuxiliaryVariable(AuxiliaryDoubleType& aux)
    {
        aux.Resize(_weights.NumColumns());
        aux.Reset();
    }

    template <typename IOElementType, bool isBiased>
    double Norm2Squared(const MatrixSolution<IOElementType, isBiased>& solution)
    {
        double result = solution.GetMatrix().ReferenceAsVector().Norm2Squared();

        if constexpr (isBiased)
        {
            result += solution.GetBias().Norm2Squared();
        }

        return result;
    }

    template <typename IOElementType, bool isBiased>
    math::RowVector<double> operator*(math::ConstRowVectorReference<IOElementType> input, const MatrixSolution<IOElementType, isBiased>& solution)
    {
        return solution.Multiply(input);
    }
} // namespace optimization
} // namespace ell

#pragma endregion implementation
