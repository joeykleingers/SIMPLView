/* ============================================================================
 * Copyright (c) 2011 Michael A. Jackson (BlueQuartz Software)
 * Copyright (c) 2011 Dr. Michael A. Groeber (US Air Force Research Laboratories)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of Michael A. Groeber, Michael A. Jackson, the US Air Force,
 * BlueQuartz Software nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  This code was written under United States Air Force Contract number
 *                           FA8650-07-D-5800
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "FindNeighborhoods.h"

#include "DREAM3DLib/Common/DREAM3DMath.h"
#include "DREAM3DLib/Common/Constants.h"
#include "DREAM3DLib/DistributionAnalysisOps/BetaOps.h"
#include "DREAM3DLib/DistributionAnalysisOps/PowerLawOps.h"
#include "DREAM3DLib/DistributionAnalysisOps/LogNormalOps.h"
#include "DREAM3DLib/StatisticsFilters/FindSizes.h"
#include "DREAM3DLib/PrivateFilters/FindBoundingBoxGrains.h"
#include "DREAM3DLib/PrivateFilters/FindGrainPhases.h"
#include "DREAM3DLib/PrivateFilters/FindGrainCentroids.h"

const static float m_pi = static_cast<float>(M_PI);

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FindNeighborhoods::FindNeighborhoods() :
AbstractFilter(),
m_GrainIds(NULL),
m_BiasedFields(NULL),
m_Phases(NULL),
m_Centroids(NULL),
m_EquivalentDiameters(NULL),
m_Neighborhoods(NULL)
{
  m_DistributionAnalysis.push_back(BetaOps::New());
  m_DistributionAnalysis.push_back(LogNormalOps::New());
  m_DistributionAnalysis.push_back(PowerLawOps::New());
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FindNeighborhoods::~FindNeighborhoods()
{
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindNeighborhoods::dataCheck(bool preflight, size_t voxels, size_t fields, size_t ensembles)
{
  setErrorCondition(0);
  std::stringstream ss;
  DataContainer* m = getDataContainer();

  GET_PREREQ_DATA(m, DREAM3D, CellData, GrainIds, ss, -301, int32_t, Int32ArrayType, voxels, 1);

  GET_PREREQ_DATA(m, DREAM3D, FieldData, EquivalentDiameters, ss, -302, float, FloatArrayType, fields, 1);
  if(getErrorCondition() == -302)
  {
	setErrorCondition(0);
	FindSizes::Pointer find_sizes = FindSizes::New();
	find_sizes->setObservers(this->getObservers());
	find_sizes->setDataContainer(getDataContainer());
	if(preflight == true) find_sizes->preflight();
	if(preflight == false) find_sizes->execute();
	GET_PREREQ_DATA(m, DREAM3D, FieldData, EquivalentDiameters, ss, -302, float, FloatArrayType, fields, 1);
  }
  GET_PREREQ_DATA(m, DREAM3D, FieldData, BiasedFields, ss, -303, bool, BoolArrayType, fields, 1);
  if(getErrorCondition() == -303)
  {
	setErrorCondition(0);
	FindBoundingBoxGrains::Pointer find_biasedfields = FindBoundingBoxGrains::New();
	find_biasedfields->setObservers(this->getObservers());
	find_biasedfields->setDataContainer(getDataContainer());
	if(preflight == true) find_biasedfields->preflight();
	if(preflight == false) find_biasedfields->execute();
	GET_PREREQ_DATA(m, DREAM3D, FieldData, BiasedFields, ss, -303, bool, BoolArrayType, fields, 1);
  }
  GET_PREREQ_DATA(m, DREAM3D, FieldData, Phases, ss, -304, int32_t, Int32ArrayType, fields, 1);
  if(getErrorCondition() == -304)
  {
	setErrorCondition(0);
	FindGrainPhases::Pointer find_grainphases = FindGrainPhases::New();
	find_grainphases->setObservers(this->getObservers());
	find_grainphases->setDataContainer(getDataContainer());
	if(preflight == true) find_grainphases->preflight();
	if(preflight == false) find_grainphases->execute();
	GET_PREREQ_DATA(m, DREAM3D, FieldData, Phases, ss, -304, int32_t, Int32ArrayType, fields, 1);
  }  
  GET_PREREQ_DATA(m, DREAM3D, FieldData, Centroids, ss, -305, float, FloatArrayType, fields, 3);
  if(getErrorCondition() == -305)
  {
	setErrorCondition(0);
	FindGrainCentroids::Pointer find_graincentroids = FindGrainCentroids::New();
	find_graincentroids->setObservers(this->getObservers());
	find_graincentroids->setDataContainer(getDataContainer());
	if(preflight == true) find_graincentroids->preflight();
	if(preflight == false) find_graincentroids->execute();
    GET_PREREQ_DATA(m, DREAM3D, FieldData, Centroids, ss, -305, float, FloatArrayType, fields, 3);
  }  
  CREATE_NON_PREREQ_DATA(m, DREAM3D, FieldData, Neighborhoods, ss, int32_t, Int32ArrayType, fields, 1);

  m_StatsDataArray = StatsDataArray::SafeObjectDownCast<IDataArray*, StatsDataArray*>(m->getEnsembleData(DREAM3D::EnsembleData::Statistics).get());
  if(m_StatsDataArray == NULL)
  {
    ss << "Stats Array Not Initialized At Beginning of '" << getNameOfClass() << "' Filter" << std::endl;
    setErrorCondition(-306);
  }

  setErrorMessage(ss.str());
}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindNeighborhoods::preflight()
{
  dataCheck(true, 1, 1, 1);
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindNeighborhoods::execute()
{
  DataContainer* m = getDataContainer();
  if (NULL == m)
  {
    setErrorCondition(-1);
    std::stringstream ss;
    ss << getNameOfClass() << " DataContainer was NULL";
    setErrorMessage(ss.str());
    return;
  }
  setErrorCondition(0);

  dataCheck(false, m->getTotalPoints(), m->getNumFieldTuples(), m->getNumEnsembleTuples());
  if (getErrorCondition() < 0)
  {
    return;
  }

  find_neighborhoods();
  notify("FindNeighborhoods Completed", 0, Observable::UpdateProgressMessage);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindNeighborhoods::find_neighborhoods()
{
  DataContainer* m = getDataContainer();
  StatsData::Pointer stats_data = StatsData::New();

  StatsDataArray& statsDataArray = *m_StatsDataArray;

  float x, y, z;
  float xn, yn, zn;
  float dx, dy, dz;
  size_t bin;
  std::vector<VectorOfFloatArray> neighborhoods;
  std::vector<std::vector<std::vector<float > > > values;
  size_t numgrains = m->getNumFieldTuples();
  size_t numensembles = m->getNumEnsembleTuples();

  neighborhoods.resize(numensembles);
  values.resize(numensembles);
  for(size_t i = 1; i < numensembles; i++)
  {
	  neighborhoods[i] = stats_data->CreateCorrelatedDistributionArrays(getDistributionType(), statsDataArray[i]->getBinNumbers()->GetSize());
	  values[i].resize(statsDataArray[i]->getBinNumbers()->GetSize());
  }

  for (size_t i = 1; i < numgrains; i++)
  {
	  m_Neighborhoods[i] = 0;
  }
  for (size_t i = 1; i < numgrains; i++)
  {
      x = m_Centroids[3*i];
      y = m_Centroids[3*i+1];
      z = m_Centroids[3*i+2];
	  if(m->getZPoints() == 1) z = 0;
      for (size_t j = i; j < numgrains; j++)
      {
	    xn = m_Centroids[3*j];
	    yn = m_Centroids[3*j+1];
	    zn = m_Centroids[3*j+2];
	    if(m->getZPoints() == 1) zn = 0;
        dx = fabs(x - xn);
        dy = fabs(y - yn);
        dz = fabs(z - zn);
        if (dx < m_EquivalentDiameters[i] && dy < m_EquivalentDiameters[i] && dz < m_EquivalentDiameters[i])
        {
            m_Neighborhoods[i]++;
        }
        if (dx < m_EquivalentDiameters[j] && dy < m_EquivalentDiameters[j] && dz < m_EquivalentDiameters[j])
        {
            m_Neighborhoods[j]++;
        }
      }
	  if(m_BiasedFields[i] == false)
	  {
		bin = size_t((m_EquivalentDiameters[i]-statsDataArray[m_Phases[i]]->getMinGrainDiameter())/statsDataArray[m_Phases[i]]->getBinStepSize());
		values[m_Phases[i]][bin].push_back(m_Neighborhoods[i]);
	  }
  }
  for (size_t i = 1; i < numensembles; i++)
  {
	  m_DistributionAnalysis[getDistributionType()]->calculateCorrelatedParameters(values[i], neighborhoods[i]);
	  statsDataArray[i]->setGrainSize_Neighbors(neighborhoods[i]);
  }
}
