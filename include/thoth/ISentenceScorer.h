#pragma once

#include <string>

class ISentenceScorer {
   public:
    virtual ~ISentenceScorer() = default;
    virtual double score(const std::string& reference, const std::string& hypothesis) const = 0;
    virtual std::string name() const = 0;
};
