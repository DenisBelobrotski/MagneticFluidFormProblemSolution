#include "Algorithm.h"
#include "../utils/Utils.h"
#include "Formulas.h"
#include "Integrals.h"

#include <iostream>
#include <algorithm>


namespace algorithm
{
	void fillVariables(Variables *variables);

	void calcBeta(Variables *variables);

	void calcRadius(Variables *variables);

	void calcHeight(Variables *variables);

	void calcIteration(Variables *variables);
}


void algorithm::fillVariables(Variables *variables)
{
	for (int i = 0; i < N + 1; i++)
	{
		variables->s[i] = LOWER_BOUND + i * STEP;

		variables->r[i] = variables->s[i];

		variables->z[N - i] = variables->s[i];

		variables->beta[i] = -M_PI_4;
	}

	variables->L = 0;
	variables->U = INITIAL_U;
	variables->B0 = INITIAL_B0;
	variables->A1 = INITIAL_A1;
	variables->A2 = INITIAL_A2;
	variables->ALPHA = INITIAL_ALPHA;
}


void algorithm::calcBeta(Variables *variables)
{
	double r0 = variables->r[0];
	double r1 = variables->r[N];

	double I0 = calcIntegral0(variables);
	double L = variables->L = calcL(I0, variables->U);
	double I1 = calcIntegral1(variables);
	double I2 = calcIntegral2(variables);

	double lowerGamma = calcLowerGamma(r0, r1, I1, I2, L, 
									   variables->U, variables->B0, 
									   variables->A1, variables->ALPHA);

#if LOG_BETA
	std::cout << "r0: " << r0 << " r1: " << r1 << std::endl;
	std::cout << "I0: " << I0 << " L: " << L << std::endl;
	std::cout << "I1: " << I1 << " I2: " << I2 << std::endl;
	std::cout << "*****************" << std::endl;
#endif

#if SIMPLE_RELAXATION_FORMULA
	std::vector<double> prevBeta = variables->beta;
#endif

	for (int i = N - 1; i > 0; i--)
	{
		double tmpZ = (variables->z[i] + variables->z[i + 1]) / 2;
		double tmpR = (variables->r[i] + variables->r[i + 1]) / 2;
		double tmpBeta = (variables->beta[i] + variables->beta[i + 1]) / 2;

		double upperPhi = calcUpperPhi(lowerGamma, I1, I2, tmpBeta, tmpR, tmpZ, L, 
									   variables->U, variables->B0, variables->A1, variables->A2);

#if SIMPLE_RELAXATION_FORMULA
		variables->beta[i] = variables->beta[i + 1] - STEP * upperPhi;
#else
		variables->beta[i] = variables->beta[i + 1] - STEP * upperPhi +
			(1 - TAU) * (variables->beta[i] - variables->beta[i + 1] + STEP * upperPhi);
#endif

#if LOG_BETA
		std::cout << "tmpR: " << tmpR << " tmpZ: " << tmpZ << std::endl;
		std::cout << "tmpBeta: " << tmpBeta << " upperPhi: " << upperPhi << std::endl;
		std::cout << "newBeta: " << variables->beta[i] << std::endl;
		std::cout << "**********" << std::endl;
#endif
	}

#if SIMPLE_RELAXATION_FORMULA
	for (int i = 0; i < N + 1; i++)
	{
		variables->beta[i] = (1 - TAU) * prevBeta[i] + TAU * variables->beta[i];
	}
#endif
}


void algorithm::calcRadius(Variables *variables)
{
	variables->r[0] = 1 / variables->L;

	for (int i = 1; i < N + 1; i++)
	{
		double tmpBeta = (variables->beta[i - 1] + variables->beta[i]) / 2;
		variables->r[i] = variables->r[i - 1] + STEP * cos(tmpBeta);
	}
}


void algorithm::calcHeight(Variables *variables)
{
	variables->z[N] = 0;

	for (int i = N - 1; i >= 0; i--)
	{
		double tmpBeta = (variables->beta[i] + variables->beta[i + 1]) / 2;
		variables->z[i] = variables->z[i + 1] - STEP * sin(tmpBeta);
	}
}


void algorithm::calcIteration(Variables *variables)
{
	calcBeta(variables);
	calcRadius(variables);
	calcHeight(variables);
}


void algorithm::calcResult(std::vector<Variables> &experimentVariables, 
						   std::vector<IterationInfo> &iterationsInfo)
{
	Variables variables;
	variables.s = std::vector<double>(N + 1);
	variables.r = std::vector<double>(N + 1);
	variables.z = std::vector<double>(N + 1);
	variables.beta = std::vector<double>(N + 1);

	fillVariables(&variables);

	double residual;
	long long iterationsCounter = 0;
	long long experimentsCounter = 0;
	IterationInfo currentIterationInfo;

	std::vector<double> prevR;
	std::vector<double> prevZ;

	Variables resultVariables = variables;

	while (variables.A2 <= 1.6)
    {
        do
        {
            prevR = variables.r;
            prevZ = variables.z;

            calcIteration(&variables);

            residual = std::max(utils::calcResidual(prevR, variables.r),
                    utils::calcResidual(prevZ, variables.z));

            iterationsCounter++;
        }
        while (residual > ACCURACY);

        resultVariables = variables;
		for (int i = 0; i < variables.r.size(); i++)
		{
			resultVariables.r[i] *= resultVariables.L;
			resultVariables.z[i] *= resultVariables.L;
		}

		experimentVariables.push_back(resultVariables);

		if (experimentsCounter % WRITE_SOLUTION_PARAM == 0)
		{
			currentIterationInfo.index = experimentsCounter;
			currentIterationInfo.u = variables.U;
			currentIterationInfo.b0 = variables.B0;
			currentIterationInfo.a1 = variables.A1;
			currentIterationInfo.a2 = variables.A2;
			currentIterationInfo.alpha = variables.ALPHA;

			iterationsInfo.push_back(currentIterationInfo);
		}

		experimentsCounter++;

		variables.A2 += 0.05;
	}

#if LOG_RESULTS
	utils::printVector(&std::cout, &(variables.s));
	std::cout << "************************" << std::endl;
	utils::printVector(&std::cout, &(variables.r));
	std::cout << "************************" << std::endl;
	utils::printVector(&std::cout, &(variables.z));
	std::cout << "************************" << std::endl;
	utils::printVector(&std::cout, &(variables.beta));
	std::cout << "************************" << std::endl;
#endif


	std::cout << "iterations count: " << iterationsCounter << std::endl;
}
