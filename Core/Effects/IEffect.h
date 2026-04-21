#pragma once

#include <string>
#include <vector>
#include <memory>

class IEffect {
public:
    virtual ~IEffect() = default;

    virtual void apply(std::vector<float>& audioBuffer) = 0;

    virtual void setParameter(const std::string& name, float value) = 0;

     virtual std::string getName() const noexcept = 0;

     virtual std::shared_ptr<IEffect> clone() const = 0;
};
