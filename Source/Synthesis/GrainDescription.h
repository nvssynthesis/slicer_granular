/*
  ==============================================================================

    GrainDescription.h
    Created: 11 Jan 2025 2:26:20pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
namespace nvs::gran {
struct GrainDescription {
	// a struct to communicate upstream (from actual realtime granular synthesis) about the coarse description of a given grain's current state
	int voice;// i wanted these 'voice' and 'grain_id' to be const but then i couldnt simply use copy constructor and don't have it in me now to redesign that
	int grain_id;
	
	double position;
	double sample_playback_rate;
	float window;
	float pan;
	bool busy;
};
}	// namespace nvs::gran
