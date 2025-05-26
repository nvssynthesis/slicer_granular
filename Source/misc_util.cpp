/*
  ==============================================================================

    misc_util.cpp
    Created: 15 Jan 2025 4:49:27pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "misc_util.h"
#include "../nvs_libraries/nvs_libraries/include/nvs_memoryless.h"
#include <random>

namespace nvs::util
{

//===============================================timbre5DPoint=================================================
bool Timbre5DPoint::operator==(Timbre5DPoint const &other) const {
	if (other.get2D() != _p2D){
		return false;
	}
	auto const other3 = other.get3D();
	for (size_t i = 0; i < _p3D.size(); ++i){
		if (_p3D[i] != other3[i]){
			return false;
		}
	}
	return true;
}

std::array<juce::uint8, 3> Timbre5DPoint::toUnsigned() const {
	for (auto p : _p3D){
		assert(inRangeM1_1(p));
	}
	using namespace nvs::memoryless;
	std::array<juce::uint8, 3> u {
		static_cast<juce::uint8>(biuni(_p3D[0]) * 255.f),
		static_cast<juce::uint8>(biuni(_p3D[1]) * 255.f),
		static_cast<juce::uint8>(biuni(_p3D[2]) * 255.f)
	};
	return u;
}

void TimbreSpaceHolder::add5DPoint(timbre2DPoint p2D, std::array<float, 3> p3D){
	using namespace nvs::util;
	assert(inRangeM1_1(p2D.getX()) && inRangeM1_1(p2D.getY())
		   && inRangeM1_1(p3D[0]) && inRangeM1_1(p3D[1]) && inRangeM1_1(p3D[2])
		   );
	timbres5D.add({
		._p2D{p2D},
		._p3D{p3D}
	});
}

void TimbreSpaceHolder::clear() {
	timbres5D.clear();
}

namespace {

int findNearestPoint(Timbre5DPoint p5D, juce::Array<Timbre5DPoint> timbres5D, float higher_3D_weight = 0.01f) {
	/*
	 Do consider that we are only using 2 dimenstions to find the nearest point even though they are of greater dimensionality.
	 */

	auto const px = p5D.get2D().getX();
	auto const py = p5D.get2D().getY();
	auto const pu = p5D.get3D()[0];
	auto const pv = p5D.get3D()[1];
	auto const pz = p5D.get3D()[2];
	
	float diff = 1e15;
	int idx = 0;
	for (int i = 0; i < timbres5D.size(); ++i){
		auto const current_x = timbres5D[i].get2D().getX();
		auto const current_y = timbres5D[i].get2D().getY();
		auto const current_u = timbres5D[i].get3D()[0];
		auto const current_v = timbres5D[i].get3D()[1];
		auto const current_z = timbres5D[i].get3D()[2];

		auto const xx = (current_x - px);
		auto const yy = (current_y - py);
		auto const uu = (current_u - pu);
		auto const vv = (current_v - pv);
		auto const zz = (current_z - pz);

		auto const tmp = xx*xx + yy*yy + higher_3D_weight*(uu*uu + vv*vv + zz*zz);
		if (tmp < diff){
			diff = tmp;
			idx = i;
			if (diff == 0.f){
				goto findNearestPointDone;
			}
		}
	}
	findNearestPointDone:
	return idx;
}

/**
 * Finds the K nearest points to `target` in `database`,
 * builds softmax-style weights over those K,
 * then either returns them all, or samples `numToPick` **without replacement**.
 *
 * @param  database        your full set of Timbre5DPoints
 * @param  K               how many nearest neighbors to consider (caps at database.size())
 * @param  numToPick       if <=0, we return all K weighted indices;
 *                         otherwise we pick exactly numToPick **without** replacement.
 * @param  sharpness       0=flat, >0 biases toward closer points
 * @param  higher3Dweight  extra weighting on the 3D portion of your distance metric
 */
std::vector<TimbreSpaceHolder::WeightedIdx> findProbabilisticPoints(
	const Timbre5DPoint&               target,
	const juce::Array<Timbre5DPoint>&  database,
	int                                K,
	int                                numToPick,
	double                             sharpness,
	float                              higher3Dweight = 0.01f)
{
	// 1) handle empty DB
	if (database.isEmpty())
		return {};

	// 2) compute squared distances
	std::vector<std::pair<double,int>> distIdx;
	distIdx.reserve (database.size());
	for (int i = 0; i < database.size(); ++i)
	{
		auto const& p = database.getReference(i);
		double dx = p.get2D().getX() - target.get2D().getX();
		double dy = p.get2D().getY() - target.get2D().getY();
		auto   u1 = p.get3D();
		auto   u2 = target.get3D();
		double du = u1[0] - u2[0],
			   dv = u1[1] - u2[1],
			   dz = u1[2] - u2[2];

		double d2 = dx*dx + dy*dy
				  + higher3Dweight * (du*du + dv*dv + dz*dz);
		distIdx.emplace_back (d2, i);
	}

	// 3) keep only the K nearest
	K = std::min<int> (K, (int) distIdx.size());
	std::nth_element (
		distIdx.begin(),
		distIdx.begin() + K,
		distIdx.end(),
		[](auto& a, auto& b){ return a.first < b.first; });
	distIdx.resize (K);

	// 4) build “softmax” weights = exp(−sharpness * (d / dmax))
	double dmax = 0;
	for (auto& di : distIdx)
		dmax = std::max (dmax, di.first);

	// if all distances zero, give uniform weights
	std::vector<double> weights;
	weights.reserve (K);
	if (dmax <= 0)
	{
		weights.assign (K, 1.0 / K);
	}
	else
	{
		for (auto& di : distIdx)
			weights.push_back (std::exp ( - sharpness * (di.first / dmax) ));

		// normalize
		double sum = std::accumulate (weights.begin(), weights.end(), 0.0);
		for (auto& w : weights)
			w /= sum;
	}

	using WeightedIdx = TimbreSpaceHolder::WeightedIdx;
	// 5) build result vector of all weighted indices
	std::vector<WeightedIdx> allW;
	allW.reserve (K);
	for (int i = 0; i < K; ++i)
		allW.push_back ({ distIdx[i].second, weights[i] });

	// 6a) if caller just wants the full distribution, return it:
	if (numToPick <= 0 || numToPick >= K)
		return allW;

	// 6b) otherwise sample numToPick *without replacement*
	//     by repeatedly drawing from the discrete distribution
	//     then zeroing out that weight and re-normalizing.
	std::vector<WeightedIdx> picked;
	picked.reserve (numToPick);

	// extract weights into their own vector
	std::vector<double> weightArr;
	weightArr.reserve(allW.size());
	for (auto& wi : allW)
		weightArr.push_back(wi.weight);
	
	std::mt19937 rng { std::random_device{}() };
	for (int pick = 0; pick < numToPick; ++pick)
	{
		// construct the discrete_distribution over the weights
		std::discrete_distribution<int> dist(weightArr.begin(), weightArr.end());

		int choice = dist(rng);
		picked.push_back(allW[choice]);

		// zero out that weight so it's never picked again
		weightArr[choice] = 0.0;

		// re-normalize the remaining weights in-place
		double rem = std::accumulate(weightArr.begin(), weightArr.end(), 0.0);
		if (rem <= 0.0)
			break;  // no weight left

		for (auto& w : weightArr)
			w /= rem;
	}

	return picked;
}
int findProbabilisticPoint(
	const Timbre5DPoint& target,
	juce::Array<Timbre5DPoint> database,
	int K,                     // how many nearest to consider
	double sharpness,          // 0 = uniform, higher favors more “nearest”
	float   higher3Dweight = 0.01f)
{
	if (database.size() == 0){
		return 0;
	}
	// compute squared distances
	std::vector<std::pair<double,int>> distIdx;
	distIdx.reserve(database.size());
	for (int i = 0; i < (int)database.size(); ++i)
	{
		auto const& p = database[i];
		double dx = p.get2D().getX() - target.get2D().getX();
		double dy = p.get2D().getY() - target.get2D().getY();
		double du = p.get3D()[0] - target.get3D()[0];
		double dv = p.get3D()[1] - target.get3D()[1];
		double dz = p.get3D()[2] - target.get3D()[2];
		double d2 = dx*dx + dy*dy + higher3Dweight*(du*du + dv*dv + dz*dz);
		distIdx.emplace_back(d2, i);
	}

	// get K smallest distances (partial sort)
	if (K < (int)distIdx.size()) {
		std::nth_element(distIdx.begin(),
						 distIdx.begin() + K,
						 distIdx.end(),
						 [](auto &a, auto &b){ return a.first < b.first; });
	}
	// shrink to  K nearest
	distIdx.resize(std::min(K, (int)distIdx.size()));

	// build weights via softmax-like mapping
	double dmax = 0.0;
	for (auto& di : distIdx) {
		dmax = std::max(dmax, di.first);
	}
	if (dmax <= 0.0) {
		return distIdx.front().second;  // if all identical, pick the zeroeth
	}
	
	std::vector<double> weights;
	weights.reserve(distIdx.size());
	for (auto& di : distIdx)
	{
		double scaled = di.first / dmax; // in [0,1]
		weights.push_back(std::exp(- sharpness * scaled));
	}
	// normalize
	double sum = std::accumulate(weights.begin(), weights.end(), 0.0);
	for (auto& w : weights) { w /= sum; }

	// draw one index from distribution
	static thread_local std::mt19937 rng{ std::random_device{}() };
	std::discrete_distribution<int> dist(weights.begin(), weights.end());
	int choice = dist(rng);

	return distIdx[choice].second;
}
}	// end anonymous namespace

void TimbreSpaceHolder::setProbabilisticPointFromTarget(const Timbre5DPoint& target, int K_neighbors, double sharpness, float higher3Dweight){
	if (timbres5D.size() == 0){
		return;
	}
	setCurrentPointIndices(findProbabilisticPoints(target, timbres5D, K_neighbors, sharpness, higher3Dweight));
}

/*
 LoggingGuts::LoggingGuts wants to be defined in the corresponding PluginProcessor that uses the logger! This way, the logFile can have an appropriate name.
 */
LoggingGuts::~LoggingGuts()
{
	fileLogger.trimFileSize(logFile , 64 * 1024);
	juce::Logger::setCurrentLogger (nullptr);
}
void LoggingGuts::logIfNaNOrInf(juce::AudioBuffer<float> buffer){
	float rms = 0.f;
	for (auto ch = 0; ch < buffer.getNumChannels(); ++ch){
		rms += buffer.getRMSLevel(ch, 0, buffer.getNumSamples());
	}
	if (rms != rms) {
		fileLogger.writeToLog("processBlock:						rms was NaN");
	}
	if (std::isinf(rms)) {
		fileLogger.writeToLog("processBlock:						rms was Inf");
	}
}

SampleManagementGuts::SampleManagementGuts()
{
	formatManager.registerBasicFormats();
}
SampleManagementGuts::~SampleManagementGuts()
{
	formatManager.clearFormats();
}

}	// namespace nvs::util
