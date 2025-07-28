#pragma once

#include <Editor/Common.h>

namespace Vortex {

class RatingEstimator {
   public:
    ~RatingEstimator();

    RatingEstimator(const char* databaseFile);

    double estimateRating();

   private:
    double* myWeights;
};

};  // namespace Vortex
