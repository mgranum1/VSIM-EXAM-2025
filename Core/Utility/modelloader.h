#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <string>
#include <memory>
#include "Modeldata.h"

namespace bbl
{
class ModelLoader
{
public:
    ModelLoader() = default;
    ~ModelLoader() = default;


    std::unique_ptr<ModelData> loadModel(const std::string& modelPath,
                                         const std::string& customTexturePath = "");

private:
    // Internal loading helper
    void loadOBJ(const std::string& modelPath,
                 ModelData& modelData,
                 const std::string& customTexturePath);
};
}

#endif // MODEL_LOADER_H
