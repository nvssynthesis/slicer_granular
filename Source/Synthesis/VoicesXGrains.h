/*
  ==============================================================================

    VoicesXGrains.h
    Created: 17 Feb 2025 2:48:46pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once

static constexpr size_t N_GRAINS =
#if defined(DEBUG_BUILD) | defined(DEBUG) | defined(_DEBUG)
								12;
#else
								14;
#endif

constexpr static int N_VOICES =
#if defined(DEBUG_BUILD) | defined(DEBUG) | defined(_DEBUG)
										2;
#else
										4;
#endif
