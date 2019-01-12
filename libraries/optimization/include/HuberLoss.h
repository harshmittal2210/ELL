////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     HuberLoss.h (optimization)
//  Authors:  Lin Xiao, Ofer Dekel
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <math/include/Vector.h>

namespace ell
{
namespace optimization
{
    /// <summary> Implements the Huber Loss function, which is a version of the absolute loss with Huber smoothing. </summary>
    class HuberLoss
    {
    public:
        /// <summary> Checks if an output is compatible with this loss. </summary>
        template <typename OutputType>
        constexpr static bool VerifyOutput(OutputType)
        {
            return true;
        }

        /// <summary> Constructor. </summary>
        ///
        /// <param name="gamma"> The inverse smoothness parameter. </param>
        HuberLoss(double gamma = 2.0) :
            _gamma(gamma) {}

        /// <summary> Returns the smoothness of this loss, which is the Lipschitz coefficient of the loss gradient. </summary>
        double Smoothness() const { return 2.0 / _gamma; }

        /// <summary> Returns the loss of a scalar prediction, given the true scalar output. </summary>
        ///
        /// <param name="prediction"> The predicted output. </param>
        /// <param name="output"> The true output. </param>
        template <typename OutputType>
        double Value(double prediction, OutputType output) const;

        /// <summary> Returns the loss derivative at a given scalar point. </summary>
        ///
        /// <param name="prediction"> The predicted output. </param>
        /// <param name="output"> The true output. </param>
        template <typename OutputType>
        double Derivative(double prediction, OutputType output) const;

        /// <summary> Returns the value of the loss conjugate at a given point. </summary>
        ///
        /// <param name="v"> The point at which to evaluate the conjugate. </param>
        /// <param name="output"> The output. </param>
        template <typename OutputType>
        double Conjugate(double v, OutputType output) const;

        /// <summary>
        /// Returns the value of the proximal operator of the conjugate of the loss, which is
        ///
        ///     argmin_b {theta*g(b) + (1/2)*(b - a)^2}
        ///
        /// where g() is the convex conjugate of f()
        /// </summary>
        ///
        /// <param name="theta"> The weight of the conjugate function in the proximal operator definition. </param>
        /// <param name="z"> The point at which the proximal operator is evaluated. </param>
        /// <param name="output"> The output. </param>
        template <typename OutputType>
        double ConjugateProx(double theta, double z, OutputType output) const;

    private:
        double _gamma;
    };
} // namespace optimization
} // namespace ell

#pragma region implementation

namespace ell
{
namespace optimization
{
    template <typename OutputType>
    double HuberLoss::Value(double prediction, OutputType output) const
    {
        double residual = prediction - output;

        if (residual >= -_gamma && residual <= _gamma)
        {
            return 0.5 / _gamma * residual * residual;
        }
        return std::abs(residual) - 0.5 * _gamma;
    }

    template <typename OutputType>
    double HuberLoss::Derivative(double prediction, OutputType output) const
    {
        double residual = prediction - output;

        if (residual >= -_gamma && residual <= _gamma)
        {
            return residual / _gamma;
        }
        if (residual > 0)
        {
            return 1.0;
        }
        return -1.0;
    }

    template <typename OutputType>
    double HuberLoss::Conjugate(double v, OutputType output) const
    {
        if (-1.0 <= v && v <= 1.0)
        {
            return output * v + 0.5 * _gamma * v * v;
        }
        return std::numeric_limits<double>::infinity();
    }

    template <typename OutputType>
    double HuberLoss::ConjugateProx(double theta, double z, OutputType output) const
    {
        double a = (z - theta * output) / (1 + theta * _gamma);

        if (a <= -1.0)
        {
            return -1.0;
        }
        if (a >= 1.0)
        {
            return 1.0;
        }
        return a;
    }
} // namespace optimization
} // namespace ell

#pragma endregion implementation
