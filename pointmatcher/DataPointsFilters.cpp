// kate: replace-tabs off; indent-width 4; indent-mode normal
// vim: ts=4:sw=4:noexpandtab
/*

Copyright (c) 2010--2011,
François Pomerleau and Stephane Magnenat, ASL, ETHZ, Switzerland
You can contact the authors at <f dot pomerleau at gmail dot com> and
<stephane at magnenat dot net>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ETH-ASL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "Core.h"
#include <stdexcept>
#include <algorithm>
#include <boost/format.hpp>

// Eigenvalues
#include "Eigen/QR"

using namespace std;
using namespace PointMatcherSupport;

// IdentityDataPointsFilter
template<typename T>
typename PointMatcher<T>::DataPoints PointMatcher<T>::IdentityDataPointsFilter::filter(
	const DataPoints& input)
{
	return input;
}

template struct PointMatcher<float>::IdentityDataPointsFilter;
template struct PointMatcher<double>::IdentityDataPointsFilter;

// MaxDistDataPointsFilter
// Constructor
template<typename T>
PointMatcher<T>::MaxDistDataPointsFilter::MaxDistDataPointsFilter(const Parameters& params):
	DataPointsFilter("MaxDistDataPointsFilter", MaxDistDataPointsFilter::availableParameters(), params),
	dim(Parametrizable::get<unsigned>("dim")),
	maxDist(Parametrizable::get<T>("maxDist"))
{
}

template<typename T>
typename PointMatcher<T>::DataPoints PointMatcher<T>::MaxDistDataPointsFilter::filter(const DataPoints& input)
{
	if (int(dim) >= input.features.rows())
		throw InvalidParameter((boost::format("MaxDistDataPointsFilter: Error, filtering on dimension number %1%, larger than feature dimensionality %2%") % dim % input.features.rows()).str());
	
	//TODO: should we do that in 2 passes or use conservativeResize?
	const int nbPointsIn = input.features.cols();
	const int nbRows = input.features.rows();
	int nbPointsOut = 0;
	if (dim == 3)
	{
		nbPointsOut = (input.features.topRows(nbRows-1).colwise().norm().array() < maxDist).count();
	}
	else
	{
		nbPointsOut = (input.features.row(dim).array().abs() < maxDist).count();
	}

	DataPoints outputCloud(
		typename DataPoints::Features(input.features.rows(), nbPointsOut),
		input.featureLabels
	);
	
	// if there is descriptors, copy the labels
	if (input.descriptors.cols() > 0)
	{
		outputCloud.descriptors = typename DataPoints::Descriptors(input.descriptors.rows(), nbPointsOut);
		outputCloud.descriptorLabels = input.descriptorLabels;
	}
	
	int j = 0;
	if(dim == 3) // Euclidian distance
	{
		for (int i = 0; i < nbPointsIn; i++)
		{
			if (input.features.col(i).head(nbRows-1).norm() < maxDist)
			{
				outputCloud.features.col(j) = input.features.col(i);
				if (outputCloud.descriptors.cols() > 0)
					outputCloud.descriptors.col(j) = input.descriptors.col(i);
				j++;
			}
		}
	}
	else // Single axis distance
	{
		for (int i = 0; i < nbPointsIn; i++)
		{
			if (anyabs(input.features(dim, i)) < maxDist)
			{
				outputCloud.features.col(j) = input.features.col(i);
				if (outputCloud.descriptors.cols() > 0)
					outputCloud.descriptors.col(j) = input.descriptors.col(i);
				j++;
			}
		}
	}
	
	assert(j == nbPointsOut);
	return outputCloud;
}

template struct PointMatcher<float>::MaxDistDataPointsFilter;
template struct PointMatcher<double>::MaxDistDataPointsFilter;

// MinDistDataPointsFilter
// Constructor
template<typename T>
PointMatcher<T>::MinDistDataPointsFilter::MinDistDataPointsFilter(const Parameters& params):
	DataPointsFilter("MinDistDataPointsFilter", MinDistDataPointsFilter::availableParameters(), params),
	dim(Parametrizable::get<unsigned>("dim")),
	minDist(Parametrizable::get<T>("minDist"))
{
}

template<typename T>
typename PointMatcher<T>::DataPoints PointMatcher<T>::MinDistDataPointsFilter::filter(const DataPoints& input)
{
	if (int(dim) >= input.features.rows())
		throw InvalidParameter((boost::format("MinDistDataPointsFilter: Error, filtering on dimension number %1%, larger than feature dimensionality %2%") % dim % input.features.rows()).str());
	
	const int nbPointsIn = input.features.cols();
	const int nbRows = input.features.rows();
	int nbPointsOut = 0;
	if (dim == 3)
	{
		nbPointsOut = (input.features.topRows(nbRows-1).colwise().norm().array() > minDist).count();
	}
	else
	{
		nbPointsOut = (input.features.row(dim).array().abs() > minDist).count();
	}

	DataPoints outputCloud(
		typename DataPoints::Features(input.features.rows(), nbPointsOut),
		input.featureLabels
	);
	
	// if there is descriptors, copy the labels
	if (input.descriptors.cols() > 0)
	{
		outputCloud.descriptors = typename DataPoints::Descriptors(input.descriptors.rows(), nbPointsOut);
		outputCloud.descriptorLabels = input.descriptorLabels;
	}
	
	int j = 0;
	if(dim == 3) // Euclidian distance
	{
		for (int i = 0; i < nbPointsIn; i++)
		{
			if (input.features.col(i).head(nbRows-1).norm() > minDist)
			{
				outputCloud.features.col(j) = input.features.col(i);
				if (outputCloud.descriptors.cols() > 0)
					outputCloud.descriptors.col(j) = input.descriptors.col(i);
				j++;
			}
		}
	}
	else // Single axis distance
	{
		for (int i = 0; i < nbPointsIn; i++)
		{
			if (anyabs(input.features(dim, i)) > minDist)
			{
				outputCloud.features.col(j) = input.features.col(i);
				if (outputCloud.descriptors.cols() > 0)
					outputCloud.descriptors.col(j) = input.descriptors.col(i);
				j++;
			}
		}
	}
	assert(j == nbPointsOut);
	return outputCloud;
}

template struct PointMatcher<float>::MinDistDataPointsFilter;
template struct PointMatcher<double>::MinDistDataPointsFilter;

// MaxQuantileOnAxisDataPointsFilter
// Constructor
template<typename T>
PointMatcher<T>::MaxQuantileOnAxisDataPointsFilter::MaxQuantileOnAxisDataPointsFilter(const Parameters& params):
	DataPointsFilter("MaxQuantileOnAxisDataPointsFilter", MaxQuantileOnAxisDataPointsFilter::availableParameters(), params),
	dim(Parametrizable::get<unsigned>("dim")),
	ratio(Parametrizable::get<T>("ratio"))
{
}

template<typename T>
typename PointMatcher<T>::DataPoints PointMatcher<T>::MaxQuantileOnAxisDataPointsFilter::filter(const DataPoints& input)
{
	if (int(dim) >= input.features.rows())
		throw InvalidParameter((boost::format("MaxQuantileOnAxisDataPointsFilter: Error, filtering on dimension number %1%, larger than feature dimensionality %2%") % dim % input.features.rows()).str());
	
	const int nbPointsIn = input.features.cols();
	const int nbPointsOut = nbPointsIn * ratio;
	
	// build array
	vector<T> values;
	values.reserve(input.features.cols());
	for (int x = 0; x < input.features.cols(); ++x)
		values.push_back(input.features(dim, x));
	
	// get quartiles value
	nth_element(values.begin(), values.begin() + (values.size() * ratio), values.end());
	const T limit = values[nbPointsOut];
	
	// build output values
	DataPoints outputCloud(
		typename DataPoints::Features(input.features.rows(), nbPointsOut),
		input.featureLabels
	);
	if (input.descriptors.cols() > 0)
	{
		outputCloud.descriptors = typename DataPoints::Descriptors(input.descriptors.rows(), nbPointsOut);
		outputCloud.descriptorLabels = input.descriptorLabels;
	}
	
	// fill output values
	int j = 0;
	for (int i = 0; i < nbPointsIn; i++)
	{
		if (input.features(dim, i) < limit)
		{
			outputCloud.features.col(j) = input.features.col(i);
			if (outputCloud.descriptors.cols() > 0)
				outputCloud.descriptors.col(j) = input.descriptors.col(i);
			j++;
		}
	}
	assert(j <= nbPointsOut);
	
	if (j < nbPointsOut)
	{
		if (outputCloud.descriptors.cols() > 0)
			return DataPoints(
				outputCloud.features.corner(Eigen::TopLeft,outputCloud.features.rows(),j),
				outputCloud.featureLabels,
				outputCloud.descriptors.corner(Eigen::TopLeft,outputCloud.descriptors.rows(),j),
				outputCloud.descriptorLabels
			);
		else
			return DataPoints(
				outputCloud.features.corner(Eigen::TopLeft,outputCloud.features.rows(),j),
				outputCloud.featureLabels
			);
	}
	
	return outputCloud;
}

template struct PointMatcher<float>::MaxQuantileOnAxisDataPointsFilter;
template struct PointMatcher<double>::MaxQuantileOnAxisDataPointsFilter;


// UniformizeDensityDataPointsFilter
// Constructor
template<typename T>
PointMatcher<T>::UniformizeDensityDataPointsFilter::UniformizeDensityDataPointsFilter(const Parameters& params):
	DataPointsFilter("UniformizeDensityDataPointsFilter", UniformizeDensityDataPointsFilter::availableParameters(), params),
	ratio(Parametrizable::get<T>("ratio")),
	nbBin(Parametrizable::get<unsigned>("nbBin"))
{
}

// Structure for histogram (created for UniformizeDensityDataPointsFilter)
struct HistElement 
{
	int count;
	int id;
	float ratio;
	HistElement():count(0), id(-1), ratio(1.0){};
	static bool largestCountFirst(const HistElement h1, const HistElement h2)
	{
		return (h1.count > h2.count);	
	};
	static bool smallestIdFirst(const HistElement h1, const HistElement h2)
	{
		return (h1.id < h2.id);	
	};
};

template<typename T>
typename PointMatcher<T>::DataPoints PointMatcher<T>::UniformizeDensityDataPointsFilter::filter(const DataPoints& input)
{
	
	const int nbPointsIn = input.features.cols();
	const int nbPointsOut = nbPointsIn * ratio;

	typename DataPoints::Descriptors origineDistance(1, nbPointsIn);
	origineDistance = input.features.colwise().norm();

	const T minDist = origineDistance.minCoeff();
	const T maxDist = origineDistance.maxCoeff();
	const T delta = (maxDist - minDist)/(T)nbBin;

	typename DataPoints::Descriptors binId(1, nbPointsIn);
	
	std::vector<HistElement> hist;
	hist.resize(nbBin);

	// Initialize ids (useful to backtrack after sorting)
	for(unsigned i=0; i < nbBin; i++)
	{
		hist[i].id = i;	
	}

	// Associate a bin per point and cumulate the histogram
	for (int i=0; i < nbPointsIn; i++)
	{
		unsigned id = (origineDistance(i)-minDist)/delta;

		// validate last interval
		if(id == nbBin)
			id = nbBin-1;

		hist[id].count = hist[id].count + 1;

		binId(i) = id;
		
	}
	
	//cout << "count:" << endl;
	//for(int i=0; i<hist.size(); i++)
	//{
	//	cout << hist[i].count << ", ";	
	//}
	
	
	// Sort histogram based on count
	std::sort(hist.begin(), hist.end(), HistElement::largestCountFirst);
	
	// Search for maximum nb points per bin respecting the ratio constraint
	int theta = 0;
	assert(nbBin>0);
	for(unsigned j=0; j < (nbBin-1); j++)
	{
		int totalDiff = 0;
		for(unsigned i=0; i <= j; i++ )
		{
			totalDiff += hist[i].count - hist[j+1].count;
		}

		if(totalDiff > nbPointsOut)
		{
			int fullBinCount = 0;
			for(unsigned i=0; i <= j; i++ )
			{
				fullBinCount += hist[i].count;
			}

			theta = ((T)fullBinCount - (ratio)*(T)nbPointsIn)/(j+1);
			break;
		}
	}

	assert(theta != 0);

	

	// Compute the acceptance ratio per bin
	for(unsigned i=0; i<nbBin ; i++)
	{
		if(hist[i].count != 0)
		{
			hist[i].ratio = (float)theta/(float)hist[i].count;
		}
		else
		{
			hist[i].ratio = 1.0;
		}
	}
	
	//cout << "ratio:" << endl;
	//for(int i=0; i<hist.size(); i++)
	//{
	//	cout << hist[i].ratio << ", ";	
	//}
	//cout << endl;
	
	// Sort back histogram based on id
	std::sort(hist.begin(), hist.end(), HistElement::smallestIdFirst);
	

	// build output values
	DataPoints outputCloud(
		typename DataPoints::Features(input.features.rows(), nbPointsIn),
		input.featureLabels
	);
	if (input.descriptors.cols() > 0)
	{
		outputCloud.descriptors = typename DataPoints::Descriptors(input.descriptors.rows(), nbPointsIn);
		outputCloud.descriptorLabels = input.descriptorLabels;
	}
	
	// fill output values
	int j = 0;
	for (int i = 0; i < nbPointsIn; i++)
	{
		const int id = binId(i);
		const float r = (float)std::rand()/(float)RAND_MAX;
		if (r < hist[id].ratio)
		{
			outputCloud.features.col(j) = input.features.col(i);
			if (outputCloud.descriptors.cols() > 0)
				outputCloud.descriptors.col(j) = input.descriptors.col(i);
			j++;
		}
	}

	// Reduce the point cloud size
	outputCloud.features.conservativeResize(Eigen::NoChange, j+1);
	if (outputCloud.descriptors.cols() > 0)
			outputCloud.descriptors.conservativeResize(Eigen::NoChange,j);
	
	return outputCloud;
}

template struct PointMatcher<float>::UniformizeDensityDataPointsFilter;
template struct PointMatcher<double>::UniformizeDensityDataPointsFilter;


// SurfaceNormalDataPointsFilter
// Constructor
template<typename T>
PointMatcher<T>::SurfaceNormalDataPointsFilter::SurfaceNormalDataPointsFilter(const Parameters& params):
	DataPointsFilter("SurfaceNormalDataPointsFilter", SurfaceNormalDataPointsFilter::availableParameters(), params),
	knn(Parametrizable::get<int>("knn")),
	epsilon(Parametrizable::get<T>("epsilon")),
	keepNormals(Parametrizable::get<bool>("keepNormals")),
	keepDensities(Parametrizable::get<bool>("keepDensities")),
	keepEigenValues(Parametrizable::get<bool>("keepEigenValues")),
	keepEigenVectors(Parametrizable::get<bool>("keepEigenVectors")),
	keepMatchedIds(Parametrizable::get<bool>("keepMatchedIds"))
{
}


// Compute
template<typename T>
typename PointMatcher<T>::DataPoints PointMatcher<T>::SurfaceNormalDataPointsFilter::filter(
	const DataPoints& input)
{
	std::cerr << "SurfaceNormalDataPointsFilter::preFilter" << std::endl;
	typedef typename DataPoints::Features Features;
	typedef typename DataPoints::Label Label;
	typedef typename DataPoints::Labels Labels;
	
	const int pointsCount(input.features.cols());
	const int featDim(input.features.rows());
	const int descDim(input.descriptors.rows());

	// Validate descriptors and labels
	int insertDim(0);
	for(unsigned int i = 0; i < input.descriptorLabels.size(); i++)
		insertDim += input.descriptorLabels[i].span;
	if (insertDim != descDim)
		throw std::runtime_error("Error, descritor labels do not match descriptor data");
	
	// Reserve memory for new descriptors
	int finalDim(insertDim);
	const int dimNormals(featDim-1);
	const int dimDensities(1);
	const int dimEigValues(featDim-1);
	const int dimEigVectors((featDim-1)*(featDim-1));
	const int dimMatchedIds(knn); 
	Labels newDescriptorLabels(input.descriptorLabels);

	if (keepNormals)
	{
		newDescriptorLabels.push_back(Label("normals", dimNormals));
		finalDim += dimNormals;
	}

	if (keepDensities)
	{
		newDescriptorLabels.push_back(Label("densities", dimDensities));
		finalDim += dimDensities;
	}

	if (keepEigenValues)
	{
		newDescriptorLabels.push_back(Label("eigValues", dimEigValues));
		finalDim += dimEigValues;
	}

	if (keepEigenVectors)
	{
		newDescriptorLabels.push_back(Label("eigVectors", dimEigVectors));
		finalDim += dimEigVectors;
	}
	
	if (keepMatchedIds)
	{
		newDescriptorLabels.push_back(Label("matchedIds", dimMatchedIds));
		finalDim += dimMatchedIds;
	}

	Matrix newDescriptors(finalDim, pointsCount);
	
	KDTreeMatcher matcher(Parameters({
		{ "knn", toParam(knn) },
		{ "epsilon", toParam(epsilon) }
	}));
	matcher.init(input);

	Matches matches(typename Matches::Dists(knn, 1), typename Matches::Ids(knn, 1));
	// Search for surrounding points
	int degenerateCount(0);
	for (int i = 0; i < pointsCount; ++i)
	{
		Vector mean(Vector::Zero(featDim-1));
		Features NN(featDim-1, knn);
		
		DataPoints singlePoint(input.features.col(i), input.featureLabels, Matrix(), Labels());
		matches = matcher.findClosests(singlePoint, DataPoints());

		// Mean of nearest neighbors (NN)
		for(int j = 0; j < int(knn); j++)
		{
			const int refIndex(matches.ids(j));
			const Vector v(input.features.block(0, refIndex, featDim-1, 1));
			NN.col(j) = v;
			mean += v;
		}

		mean /= knn;
		
		// Covariance of nearest neighbors
		for (int j = 0; j < int(knn); ++j)
		{
			//std::cout << "NN.col(j):\n" << NN.col(j) << std::endl;
			//std::cout << "mean:\n" << mean << std::endl;
			NN.col(j) -= mean;
		}
		
		const Matrix C(NN * NN.transpose());
		Vector eigenVa = Vector::Identity(featDim-1, 1);
		Matrix eigenVe = Matrix::Identity(featDim-1, featDim-1);
		// Ensure that the matrix is suited for eigenvalues calculation
		if(C.fullPivHouseholderQr().rank() == featDim-1)
		{
			
			const Eigen::EigenSolver<Matrix> solver(C);
			eigenVa = solver.eigenvalues().real();
			eigenVe = solver.eigenvectors().real();
			
			//eigenVa = Eigen::EigenSolver<Matrix>(C).eigenvalues().real();
			//eigenVe = Eigen::EigenSolver<Matrix>(C).eigenvectors().real();
		}
		else
		{
			//TODO: solve without noise..
			//std::cout << "WARNING: Matrix C needed for eigen decomposition is degenerated. Expected cause: no noise in data" << std::endl;
			++degenerateCount;
		}
		
		int posCount(insertDim);
		if(keepNormals)
		{
			// Keep the smallest eigenvector as surface normal
			int smallestId(0);
			T smallestValue(numeric_limits<T>::max());
			for(int j = 0; j < dimNormals; j++)
			{
				if (eigenVa(j) < smallestValue)
				{
					smallestId = j;
					smallestValue = eigenVa(j);
				}
			}
			
			newDescriptors.block(posCount, i, dimNormals, 1) = 
			//eigenVe.row(smallestId).transpose();
			eigenVe.col(smallestId);
			posCount += dimNormals;
		}

		if(keepDensities)
		{
			//TODO: set lambda to a realistic value (based on sensor)
			//TODO: debug here: volume too low 
			//TODO: change name epsilon to avoid confusion with kdtree
			const double epsilon(0.005);

			//T volume(eigenVa(0)+lambda);
			T volume(eigenVa(0));
			for(int j = 1; j < eigenVa.rows(); j++)
			{
				volume *= eigenVa(j);
			}
			newDescriptors(posCount, i) = knn/(volume + epsilon);
			posCount += dimDensities;
		}

		if(keepEigenValues)
		{
			newDescriptors.block(posCount, i, featDim-1, 1) = 
				eigenVa;
			posCount += dimEigValues;
		}
		
		if(keepEigenVectors)
		{
			for(int k=0; k < (featDim-1); k++)
			{
				newDescriptors.block(
					posCount + k*(featDim-1), i, (featDim-1), 1) = 
						(eigenVe.row(k).transpose().cwise() * eigenVa);
			}
			
			posCount += dimEigVectors;
		}
		
		if(keepMatchedIds)
		{
			// BUG: cannot used .cast<T>() on dynamic matrices...
			for(int k=0; k < dimMatchedIds; k++)
			{
				newDescriptors(posCount + k, i) = (T)matches.ids(k);
			}
			
			posCount += dimMatchedIds;
		}
	}
	if (degenerateCount)
	{
		LOG_WARNING_STREAM("WARNING: Matrix C needed for eigen decomposition was degenerated in " << degenerateCount << " points over " << pointsCount << " (" << float(degenerateCount)*100.f/float(pointsCount) << " %)");
	}
	
	return DataPoints(input.features, input.featureLabels, newDescriptors, newDescriptorLabels);
}

template struct PointMatcher<float>::SurfaceNormalDataPointsFilter;
template struct PointMatcher<double>::SurfaceNormalDataPointsFilter;


// SamplingSurfaceNormalDataPointsFilter

// Constructor
template<typename T>
PointMatcher<T>::SamplingSurfaceNormalDataPointsFilter::SamplingSurfaceNormalDataPointsFilter(const Parameters& params):
	DataPointsFilter("SamplingSurfaceNormalDataPointsFilter", SamplingSurfaceNormalDataPointsFilter::availableParameters(), params),
	binSize(Parametrizable::get<int>("binSize")),
	averageExistingDescriptors(Parametrizable::get<bool>("averageExistingDescriptors")),
	keepNormals(Parametrizable::get<bool>("keepNormals")),
	keepDensities(Parametrizable::get<bool>("keepDensities")),
	keepEigenValues(Parametrizable::get<bool>("keepEigenValues")),
	keepEigenVectors(Parametrizable::get<bool>("keepEigenVectors"))
{
}

// Compute
template<typename T>
typename PointMatcher<T>::DataPoints PointMatcher<T>::SamplingSurfaceNormalDataPointsFilter::filter(
	const DataPoints& input)
{
	//std::cerr << "SamplingSurfaceNormalDataPointsFilter::preFilter " << input.features.cols() << std::endl;
	typedef typename DataPoints::Features Features;
	typedef typename DataPoints::Label Label;
	typedef typename DataPoints::Labels Labels;
	
	const int pointsCount(input.features.cols());
	const int featDim(input.features.rows());
	const int descDim(input.descriptors.rows());
	
	//if (pointsCount == 0)
	//	throw ConvergenceError("no point to filter");
	
	int insertDim(0);
	if (averageExistingDescriptors)
	{
		// Validate descriptors and labels
		for(unsigned int i = 0; i < input.descriptorLabels.size(); i++)
			insertDim += input.descriptorLabels[i].span;
		if (insertDim != descDim)
			throw std::runtime_error("Error, descriptor labels do not match descriptor data");
	}
	
	// Reserve memory for new descriptors
	int finalDescDim(insertDim);
	const int dimNormals(featDim-1);
	const int dimDensities(1);
	const int dimEigValues(featDim-1);
	const int dimEigVectors((featDim-1)*(featDim-1));
	Labels outputDescriptorLabels(input.descriptorLabels);

	if (keepNormals)
	{
		outputDescriptorLabels.push_back(Label("normals", dimNormals));
		finalDescDim += dimNormals;
	}

	if (keepDensities)
	{
		outputDescriptorLabels.push_back(Label("densities", dimDensities));
		finalDescDim += dimDensities;
	}

	if (keepEigenValues)
	{
		outputDescriptorLabels.push_back(Label("eigValues", dimEigValues));
		finalDescDim += dimEigValues;
	}

	if (keepEigenVectors)
	{
		outputDescriptorLabels.push_back(Label("eigVectors", dimEigVectors));
		finalDescDim += dimEigVectors;
	}
	
	// we keep build data on stack for reentrant behaviour
	BuildData buildData(input.features, input.descriptors, finalDescDim);
	
	// build the new point cloud
	buildNew(
		buildData,
		0,
		pointsCount, 
		input.features.rowwise().minCoeff(),
		input.features.rowwise().maxCoeff()
	);
	
	//std::cerr << "SamplingSurfaceNormalDataPointsFilter::preFilter done " << buildData.outputInsertionPoint << std::endl;
	
	// return the new point cloud
	return DataPoints(
		buildData.outputFeatures.corner(Eigen::TopLeft, buildData.outputFeatures.rows(), buildData.outputInsertionPoint),
		input.featureLabels,
		buildData.outputDescriptors.corner(Eigen::TopLeft, buildData.outputDescriptors.rows(), buildData.outputInsertionPoint),
		outputDescriptorLabels
	);
}


template<typename T>
size_t argMax(const typename PointMatcher<T>::Vector& v)
{
	T maxVal(0);
	size_t maxIdx(0);
	for (int i = 0; i < v.size(); ++i)
	{
		if (v[i] > maxVal)
		{
			maxVal = v[i];
			maxIdx = i;
		}
	}
	return maxIdx;
}

template<typename T>
void PointMatcher<T>::SamplingSurfaceNormalDataPointsFilter::buildNew(BuildData& data, const int first, const int last, const Vector minValues, const Vector maxValues) const
{
	const int count(last - first);
	if (count <= int(binSize))
	{
		// compute for this range
		fuseRange(data, first, last);
		return;
	}
	
	// find the largest dimension of the box
	const int cutDim = argMax<T>(maxValues - minValues);
	
	// compute number of elements
	const int rightCount(count/2);
	const int leftCount(count - rightCount);
	assert(last - rightCount == first + leftCount);
	
	//cerr << "cutting on dim " << cutDim << " at " << leftCount << endl;
	
	// sort, hack std::nth_element
	std::nth_element(
		data.indices.begin() + first,
		data.indices.begin() + first + leftCount,
		data.indices.begin() + last,
		CompareDim(cutDim, data)
	);
	
	// get value
	const int cutIndex(data.indices[first+leftCount]);
	const T cutVal(data.inputFeatures(cutDim, cutIndex));
	
	// update bounds for left
	Vector leftMaxValues(maxValues);
	leftMaxValues[cutDim] = cutVal;
	// update bounds for right
	Vector rightMinValues(minValues);
	rightMinValues[cutDim] = cutVal;
	
	// recurse
	buildNew(data, first, first + leftCount, minValues, leftMaxValues);
	buildNew(data, first + leftCount, last, rightMinValues, maxValues);
}

template<typename T>
void PointMatcher<T>::SamplingSurfaceNormalDataPointsFilter::fuseRange(BuildData& data, const int first, const int last) const
{
	const int colCount(last-first);
	const int featDim(data.inputFeatures.rows());
	assert(featDim == data.outputFeatures.rows());
	
	// build nearest neighbors list
	Matrix d(featDim-1, colCount);
	for (int i = 0; i < colCount; ++i)
		d.col(i) = data.inputFeatures.block(0,data.indices[first+i],featDim-1, 1);
	const Vector& mean = d.rowwise().sum() / T(colCount);
	const Matrix& NN = (d.colwise() - mean);
	data.outputFeatures.block(0, data.outputInsertionPoint, featDim-1,1) = mean;
	data.outputFeatures(featDim-1, data.outputInsertionPoint) = 1;
	
	// compute covariance
	/*const Eigen::Matrix<T, 3, 3> C(NN * NN.transpose());
	Eigen::Matrix<T, 3, 1> eigenVa = Eigen::Matrix<T, 3, 1>::Identity();
	Eigen::Matrix<T, 3, 3> eigenVe = Eigen::Matrix<T, 3, 3>::Identity();*/
	const Matrix C(NN * NN.transpose());
	Vector eigenVa = Vector::Identity(featDim-1, 1);
	Matrix eigenVe = Matrix::Identity(featDim-1, featDim-1);
	// Ensure that the matrix is suited for eigenvalues calculation
	if(C.fullPivHouseholderQr().rank() == featDim-1)
	{
		Eigen::EigenSolver<Matrix> solver(C);
		
		eigenVa = solver.eigenvalues().real();
		eigenVe = solver.eigenvectors().real();
		//eigenVa = Eigen::EigenSolver<Eigen::Matrix<T,3,3> >(C).eigenvalues().real();
		//eigenVe = Eigen::EigenSolver<Eigen::Matrix<T,3,3> >(C).eigenvectors().real();
	}
	else
	{
		return;
		//std::cout << "WARNING: Matrix C needed for eigen decomposition is degenerated. Expected cause: no noise in data" << std::endl;
	}
	
	// average the existing descriptors
	int insertDim(0);
	if (averageExistingDescriptors && (data.inputDescriptors.rows() != 0))
	{
		Vector newDesc(data.inputDescriptors.rows());
		for (int i = 0; i < colCount; ++i)
			newDesc += data.inputDescriptors.block(0,data.indices[first+i],data.inputDescriptors.rows(), 1);
		data.outputDescriptors.block(0, data.outputInsertionPoint, data.inputDescriptors.rows(), 1) =
			newDesc / T(colCount);
		insertDim += data.inputDescriptors.rows();
		/*cerr << "averaging desc:\n";
		cerr << data.inputDescriptors.block(0, first, data.inputDescriptors.rows(), colCount) << endl;
		cerr << data.inputDescriptors.block(0, first, data.inputDescriptors.rows(), colCount).rowwise().sum() << endl;
		cerr << data.outputDescriptors.block(0, data.outputInsertionPoint, data.inputDescriptors.rows(), 1) << endl;*/
	}
	
	if(keepNormals)
	{
		const int dimNormals(featDim-1);
		// Keep the smallest eigenvector as surface normal
		int smallestId(0);
		T smallestValue(numeric_limits<T>::max());
		for(int j = 0; j < dimNormals; j++)
		{
			if (eigenVa(j) < smallestValue)
			{
				smallestId = j;
				smallestValue = eigenVa(j);
			}
		}
		data.outputDescriptors.block(insertDim, data.outputInsertionPoint, dimNormals, 1) =
			eigenVe.col(smallestId);
		insertDim += dimNormals;
	}

	if(keepDensities)
	{
		//TODO: set lambda to a realistic value (based on sensor)
		//TODO: debug here: volume too low 
		const T epsilon(0.005);

		//T volume(eigenVa(0)+lambda);
		T volume(eigenVa(0));
		for(int j = 1; j < eigenVa.rows(); j++)
		{
			volume *= eigenVa(j);
		}
		data.outputDescriptors(insertDim, data.outputInsertionPoint) =
			T(colCount)/(volume + epsilon);
		insertDim += 1;
	}
	
	if(keepEigenValues)
	{
		const int dimEigValues(featDim-1);
		data.outputDescriptors.block(insertDim, data.outputInsertionPoint, dimEigValues, 1) = 
			eigenVa;
		insertDim += dimEigValues;
	}
	
	if(keepEigenVectors)
	{
		const int dimEigVectors((featDim-1)*(featDim-1));
		for(int k=0; k < (featDim-1); k++)
		{
			data.outputDescriptors.block(
				insertDim +  k*(featDim-1), data.outputInsertionPoint, (featDim-1), 1) = 
					(eigenVe.row(k).transpose().cwise() * eigenVa);
		}
		insertDim += dimEigVectors;
	}
	
	++data.outputInsertionPoint;
}

template struct PointMatcher<float>::SamplingSurfaceNormalDataPointsFilter;
template struct PointMatcher<double>::SamplingSurfaceNormalDataPointsFilter;


// OrientNormalsDataPointsFilter
// Compute
template<typename T>
typename PointMatcher<T>::DataPoints PointMatcher<T>::OrientNormalsDataPointsFilter::filter(const DataPoints& input)
{
	Matrix normals = input.getDescriptorByName("normals");

	if (normals.cols() == 0)
	{
		cerr << "Warning: cannot find normals in descriptors" << endl;
		return input;
	}
	
	const int nbPoints = input.features.cols();
	const int nbNormals = input.descriptors.cols();
	assert(nbPoints == nbNormals);

	Matrix points = input.features.block(0, 0, 3, nbPoints);
	
	double scalar;
	for (int i = 0; i < nbPoints; i++)
	{
		// Check normal orientation
		Vector3 vecP = -points.col(i);
		Vector3 vecN = normals.col(i);
		scalar = vecP.dot(vecN);
		//if (scalar < 0)
		if (scalar > 0)
		{
			// Swap normal
			normals.col(i) = -vecN;
		}
	}

	// Update descriptor
	DataPoints output = input;
	int row(0);
	for (unsigned int j = 0; j < input.descriptorLabels.size(); j++)
	{
		const int span(input.descriptorLabels[j].span);
		if (input.descriptorLabels[j].text.compare("normals") == 0)
		{
			output.descriptors.block(row, 0, span, nbNormals) = normals;
			break;
		}
		row += span;
	}

	return output;
}

template struct PointMatcher<float>::OrientNormalsDataPointsFilter;
template struct PointMatcher<double>::OrientNormalsDataPointsFilter;


// RandomSamplingDataPointsFilter
// Constructor
template<typename T>
PointMatcher<T>::RandomSamplingDataPointsFilter::RandomSamplingDataPointsFilter(const Parameters& params):
	DataPointsFilter("RandomSamplingDataPointsFilter", RandomSamplingDataPointsFilter::availableParameters(), params),
	prob(Parametrizable::get<double>("prob"))
{
}

// Sampling
template<typename T>
typename PointMatcher<T>::DataPoints PointMatcher<T>::RandomSamplingDataPointsFilter::randomSample(const DataPoints& input) const
{
	Eigen::Matrix<double, 1, Eigen::Dynamic> filter;
	filter.setRandom(input.features.cols());
	filter = (filter.cwise() + 1)/2;

	const int nbPoints = (filter.cwise() < prob).count();

	//std::cout << "RandomSampling: size before: " << input.features.cols() << std::endl;

	typename DataPoints::Features filteredFeat(input.features.rows(), nbPoints);


	int j(0);
	for(int i = 0; i < filter.cols(); i++)
	{
		if(filter(i) < prob)
		{
			filteredFeat.col(j) = input.features.col(i);
			j++;
		}
	}

	// To handle no descriptors
	typename DataPoints::Descriptors filteredDesc;
	if(input.descriptors.cols() > 0)
	{
		filteredDesc = typename DataPoints::Descriptors(input.descriptors.rows(),nbPoints);
		
		int k(0);
		for(int i = 0; i < filter.cols(); i++)
		{
			if(filter(i) < prob)
			{
				filteredDesc.col(k) = input.descriptors.col(i);
				k++;
			}
		}
	}

	//std::cout << "RandomSampling: size after: " << filteredFeat.cols() << std::endl;
	
	return DataPoints(filteredFeat, input.featureLabels, filteredDesc, input.descriptorLabels);
}

// Pre filter
template<typename T>
typename PointMatcher<T>::DataPoints PointMatcher<T>::RandomSamplingDataPointsFilter::filter(const DataPoints& input)
{
	return randomSample(input);
}

template struct PointMatcher<float>::RandomSamplingDataPointsFilter;
template struct PointMatcher<double>::RandomSamplingDataPointsFilter;


// FixstepSamplingDataPointsFilter
// Constructor
template<typename T>
PointMatcher<T>::FixstepSamplingDataPointsFilter::FixstepSamplingDataPointsFilter(const Parameters& params):
	DataPointsFilter("FixstepSamplingDataPointsFilter", FixstepSamplingDataPointsFilter::availableParameters(), params),
	startStep(Parametrizable::get<double>("startStep")),
	endStep(Parametrizable::get<double>("endStep")),
	stepMult(Parametrizable::get<double>("stepMult")),
	step(startStep)
{
	cout << "Using FixstepSamplingDataPointsFilter with startStep=" << startStep << ", endStep=" << endStep << ", stepMult=" << stepMult << endl; 
}


template<typename T>
void PointMatcher<T>::FixstepSamplingDataPointsFilter::init()
{
	step = startStep;
}

// Sampling
template<typename T>
typename PointMatcher<T>::DataPoints PointMatcher<T>::FixstepSamplingDataPointsFilter::fixstepSample(const DataPoints& input)
{
	const int iStep(step);
	//cerr << "FixstepSamplingDataPointsFilter::filter: stepping " << iStep << endl;
	const int nbPointsIn = input.features.cols();
	const int phase(rand() % iStep);
	const int nbPointsOut = ((nbPointsIn - phase) + iStep - 1) / iStep;
	typename DataPoints::Features filteredFeat(input.features.rows(), nbPointsOut);
	
	int j(0);
	for (int i = 0; i < nbPointsIn-phase; i++)
	{
		if ((i % iStep) == 0)
		{
			filteredFeat.col(j) = input.features.col(i+phase);
			j++;
		}
	}
	assert(j == nbPointsOut);

	// To handle no descriptors
	typename DataPoints::Descriptors filteredDesc;
	if (input.descriptors.cols() > 0)
	{
		filteredDesc = typename DataPoints::Descriptors(input.descriptors.rows(), nbPointsOut);
		
		j = 0;
		for (int i = 0; i < nbPointsIn-phase; i++)
		{
			if ((i % iStep) == 0)
			{
				filteredDesc.col(j) = input.descriptors.col(i+phase);
				j++;
			}
		}
		assert(j == nbPointsOut);
	}
	
	const double deltaStep(startStep * stepMult - startStep);
	step *= stepMult;
	if (deltaStep < 0 && step < endStep)
		step = endStep;
	if (deltaStep > 0 && step > endStep)
		step = endStep;
	
	return DataPoints(filteredFeat, input.featureLabels, filteredDesc, input.descriptorLabels);
}

// Pre filter
template<typename T>
typename PointMatcher<T>::DataPoints PointMatcher<T>::FixstepSamplingDataPointsFilter::filter(const DataPoints& input)
{
	return fixstepSample(input);
}

template struct PointMatcher<float>::FixstepSamplingDataPointsFilter;
template struct PointMatcher<double>::FixstepSamplingDataPointsFilter;

