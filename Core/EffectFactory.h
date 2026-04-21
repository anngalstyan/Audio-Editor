#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include "Effects/IEffect.h"
#include "Logging/ILogger.h"

class EffectFactory {
public:
    using Creator = std::function<std::shared_ptr<IEffect>(std::shared_ptr<ILogger>)>;
    
    static void registerEffect(const std::string& type, Creator creator);

     static std::shared_ptr<IEffect> createEffect(
        const std::string& effectType, 
        std::shared_ptr<ILogger> logger);
    
     static bool isRegistered(const std::string& type);
    
     static std::vector<std::string> getRegisteredTypes();
    
    static void clearRegistry();
    
private:
    static std::map<std::string, Creator>& getRegistry();
};
