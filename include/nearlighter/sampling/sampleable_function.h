#ifndef NEARLIGHTER_SAMPLING_SAMPLEABLE_FUNCTION_H
#define NEARLIGHTER_SAMPLING_SAMPLEABLE_FUNCTION_H

class Sampler;

/**
 * Conditional Function with a Sampling Proposal
 *
 * - evaluate() describes the target function F.
 * - PDF() and sample() describe the same conditional proposal q.
 * - Concrete interfaces define coordinates, query measures, and result metadata.
 * - Continuous queries may exclude discrete events represented by Result.
 */
template<class Condition, class Point, class Value, class Result>
class SampleableFunction {
public:
    virtual ~SampleableFunction() = default;

    /** Target Function Value; Does Not Advance the Sample Stream */
    virtual Value evaluate(const Condition& condition, const Point& point) const = 0;

    /** Continuous Proposal Density in the Concrete Interface's Measure */
    virtual float PDF(const Condition& condition, const Point& point) const = 0;

    /**
     * Proposal Sample
     *
     * The caller owns the sample stream; the function consumes it as needed.
     * Result defines sampled values, event probabilities, and any failure representation.
     * Discrete coefficients need not be recoverable through continuous evaluate().
     * A rejected proposal contributes zero; callers must not retry it until success.
     */
    virtual Result sample(
        const Condition& condition, Sampler& sampler) const = 0;
};

#endif  // NEARLIGHTER_SAMPLING_SAMPLEABLE_FUNCTION_H
