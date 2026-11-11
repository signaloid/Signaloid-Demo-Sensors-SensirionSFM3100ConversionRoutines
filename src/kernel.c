/*
 *	Copyright (c) 2024-2026, Signaloid.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a copy
 *	of this software and associated documentation files (the "Software"), to deal
 *	in the Software without restriction, including without limitation the rights
 *	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *	copies of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included in all
 *	copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 */

#include <math.h>
#include <uxhw.h>
#include "kernel.h"
#include "sensirion-sfm3100-uxhw.h"
#include "sensirion-sfm3100-monte-carlo.h"


double
SensirionSFM3100_calculateOutput(double * inputVariables, double *  outputVariables)
{
	double  Uv;
	double  calibratedValue;

	Uv              = inputVariables[kSensirionSFM3100InputVariableIndexUv];
	calibratedValue = (Uv - (kSensirionSFM3100SensorCalibrationConstantSFM3100constOffset / kSensirionSFM3100SensorCalibrationConstantSFM3100constB)) *
	                  (kSensirionSFM3100SensorCalibrationConstantSFM3100constA / kSensirionSFM3100SensorCalibrationConstantSFM3100constB) *
	                  fabs(Uv - (kSensirionSFM3100SensorCalibrationConstantSFM3100constOffset / kSensirionSFM3100SensorCalibrationConstantSFM3100constB));
	outputVariables[kSensirionSFM3100OutputVariableIndexCalibratedFlow] = calibratedValue;

	return calibratedValue;
}

double
SensirionSFM3100SingleEvaluation(double * outputVariables)
{
	double inputVariables[kSensirionSFM3100InputVariableIndexMax];

	/*
	 *	Set the input distribution value. `UxHwDoubleUniformDist()` returns a
	 *	`double` that also has associated with it a complete probability
	 *	distribution in UxHw mode, and a single random sample in Monte Carlo
	 *	(compat) mode.
	 */
	inputVariables[kSensirionSFM3100InputVariableIndexUv] = UxHwDoubleUniformDist(
		kSensirionSFM3100DefaultInputDistributionUvUniformDistLow,
		kSensirionSFM3100DefaultInputDistributionUvUniformDistHigh
	);

	return SensirionSFM3100_calculateOutput(inputVariables, outputVariables);
}

double
SensirionSFM3100CalculateOutputUxHw(
	double *    outputVariables,
	double *    monteCarloOutputSamples)
{
	double result;

	result = sensirionSFM3100UxHw(outputVariables);
	monteCarloOutputSamples[0] = result;

	return result;
}

double
SensirionSFM3100CalculateOutputMonteCarlo(
	size_t      numberOfMonteCarloIterations,
	double *    outputVariables,
	double *    monteCarloOutputSamples)
{
	return sensirionSFM3100MonteCarlo(
		numberOfMonteCarloIterations,
		outputVariables,
		monteCarloOutputSamples
	);
}
