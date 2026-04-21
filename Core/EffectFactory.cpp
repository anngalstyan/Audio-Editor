#include "EffectFactory.h"

std::map<std::string, EffectFactory::Creator>& EffectFactory::getRegistry() {
    static std::map<std::string, Creator> registry;
    return registry;
}

void EffectFactory::registerEffect(const std::string& type, Creator creator) {
    if (type.empty()) {
        return;
    }
    getRegistry()[type] = std::move(creator);
}

std::shared_ptr<IEffect> EffectFactory::createEffect(const std::string& effectType, 
                                                     std::shared_ptr<ILogger> logger) {
    if (effectType.empty()) {
        if (logger) {
            logger->error("EffectFactory: Empty effect type requested");
        }
        return nullptr;
    }
    
    auto& registry = getRegistry();
    const auto it = registry.find(effectType);
    
    if (it != registry.end()) {
        try {
            return it->second(std::move(logger));
        } catch (const std::exception& e) {
            if (logger) {
                logger->error("EffectFactory: Failed to create effect '" + effectType + "': " + e.what());
            }
            return nullptr;
        }
    }
    
    if (logger) {
        logger->error("Unknown effect type: " + effectType);
    }
    
    return nullptr;
}

bool EffectFactory::isRegistered(const std::string& type) {
    return getRegistry().find(type) != getRegistry().end();
}

std::vector<std::string> EffectFactory::getRegisteredTypes() {
    std::vector<std::string> types;
    auto& registry = getRegistry();
    types.reserve(registry.size());
    
    for (const auto& pair : registry) {
        types.push_back(pair.first);
    }
    
    return types;
}

void EffectFactory::clearRegistry() {
    getRegistry().clear();
}
