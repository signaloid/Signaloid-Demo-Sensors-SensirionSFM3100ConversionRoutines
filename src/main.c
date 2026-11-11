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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include "utilities.h"
#include "kernel.h"
#include "common.h"

#ifdef NO_OS_AVAILABLE

void
returnZeroNoOS(void);
#endif

int
main(int argc, char *  argv[])
{
	CommandLineArguments        arguments;
	double *                    monteCarloOutputSamples = NULL;
	clock_t                     start               = 0;
	clock_t                     end                 = 0;
	double                      cpuTimeUsedSeconds  = 0.0;
	double                      outputVariables[kSensirionSFM3100OutputVariableIndexMax] = { 0 };
	const char *                outputVariableNames[kSensirionSFM3100OutputVariableIndexMax] = {
		[kSensirionSFM3100OutputVariableIndexCalibratedFlow] = "calibratedSensorOutput",
	};
	const char *                outputVariableDescriptions[kSensirionSFM3100OutputVariableIndexMax] = {
		[kSensirionSFM3100OutputVariableIndexCalibratedFlow] = "Calibrated Air Flow (SLM)",
	};
	kOutputVariableTypeIndex    outputVariableTypes[kSensirionSFM3100OutputVariableIndexMax] = {
		[kSensirionSFM3100OutputVariableIndexCalibratedFlow] = kOutputVariableTypeDistribution,
	};
	const char *                applicationDescription = "Sensirion SFM3100 Conversion Routines";
	MeanAndVariance             meanAndVariance;

	/*
	 *	Get command-line arguments. In the no-OS build there is no command
	 *	line, so this instead applies the hard-coded configuration in
	 *	`setNoOSCommandLineArguments()`.
	 */
	if (getCommandLineArguments(argc, argv, &arguments))
	{
		return kCommonConstantReturnTypeError;
	}

	/*
	 *	MonteCarlo output samples are used even in the UxHw use case to store
	 *	the result of the single distributional evaluation.
	 */
	monteCarloOutputSamples = (double *) checkedMalloc(
		(arguments.common.numberOfMonteCarloIterations > 0 ? arguments.common.numberOfMonteCarloIterations : 1) * sizeof(double),
		__FILE__,
		__LINE__
	);

	const bool isScalarOutputSelected = isSelectedOutputScalar(
		&arguments.common,
		outputVariableTypes,
		kSensirionSFM3100OutputVariableIndexMax
	);

	/*
	 *	Start timing.
	 */
	if (arguments.common.isTimingEnabled)
	{
		start = clock();
	}

	/*
	 *	Dispatch to the mode-specific kernel.
	 *	`SensirionSFM3100CalculateOutputMonteCarlo` forwards to
	 *	`sensirionSFM3100MonteCarlo`, which runs the Monte Carlo loop;
	 *	`SensirionSFM3100CalculateOutputUxHw` forwards to
	 *	`sensirionSFM3100UxHw`, which runs a single distributional evaluation.
	 *	Both wrappers take what they need as scalars — the UxHw path needs
	 *	nothing at all — so this is the only place that unpacks
	 *	`CommandLineArguments` for the kernel.
	 */
	if (arguments.common.isMonteCarloMode)
	{
		SensirionSFM3100CalculateOutputMonteCarlo(
			arguments.common.numberOfMonteCarloIterations,
			outputVariables,
			monteCarloOutputSamples
		);

		/*
		 *	If not doing the UxHw version, then approximate the cost of the
		 *	third phase of Monte Carlo (post-processing), by calculating the
		 *	mean and variance.
		 */
		if (!isScalarOutputSelected)
		{
			meanAndVariance = calculateMeanAndVarianceOfDoubleSamples(
				monteCarloOutputSamples,
				arguments.common.numberOfMonteCarloIterations
			);
			outputVariables[kSensirionSFM3100OutputVariableIndexCalibratedFlow] = meanAndVariance.mean;
		}
	}
	else
	{
		SensirionSFM3100CalculateOutputUxHw(outputVariables, monteCarloOutputSamples);
	}

	/*
	 *	Stop timing.
	 */
	if (arguments.common.isTimingEnabled)
	{
		end                 = clock();
		cpuTimeUsedSeconds  = ((double) (end - start)) / CLOCKS_PER_SEC;
	}

	/*
	 *	Print the results (either in JSON or standard output format).
	 */
	if (arguments.common.isOutputJSONMode)
	{
		printJSONFormattedOutput(
			&arguments.common,
			monteCarloOutputSamples,
			outputVariables,
			outputVariableNames,
			kSensirionSFM3100OutputVariableIndexMax,
			applicationDescription,
			outputVariableTypes
		);
	}
	else
	{
		printHumanConsumableOutput(
			&arguments.common,
			kSensirionSFM3100OutputVariableIndexMax,
			outputVariables,
			outputVariableNames,
			outputVariableDescriptions,
			monteCarloOutputSamples,
			outputVariableTypes
		);
	}

	/*
	 *	Print timing result.
	 */
	if (arguments.common.isTimingEnabled)
	{
		printf("\nCPU time used: %lf seconds\n", cpuTimeUsedSeconds);
	}

	/*
	 *	Write output data.
	 */
	if (arguments.common.isWriteToFileEnabled)
	{
		if (writeOutputDoubleDistributionsToCSV(
				arguments.common.outputFilePath,
				outputVariables,
				outputVariableNames,
				kSensirionSFM3100OutputVariableIndexMax
		))
		{
			return kCommonConstantReturnTypeError;
		}
	}

	/*
	 *	Save Monte Carlo outputs in an output file.
	 */
	if (arguments.common.isMonteCarloMode)
	{
		saveMonteCarloDoubleDataToDataDotOutFile(
			monteCarloOutputSamples, (uint64_t) (cpuTimeUsedSeconds * 1000000),
			arguments.common.numberOfMonteCarloIterations
		);
	}

	/*
	 *	Free dynamically-allocated memory.
	 */
	free(monteCarloOutputSamples);

#ifdef NO_OS_AVAILABLE
	returnZeroNoOS();
#else

	return 0;

#endif
}
