#include <iostream>
#include "OpenMfx/Sdk/Cpp/Host/Host"
#include "OpenMfx/Sdk/Cpp/Host/EffectRegistry"
#include "OpenMfx/Sdk/Cpp/Host/EffectLibrary"
#include "OpenMfx/Sdk/Cpp/Host/MeshEffect"
#include "OpenMfx/Sdk/Cpp/Host/MeshProps"
#include "OpenMfx/Sdk/Cpp/Host/Mesh"

int main() {
    OpenMfx::Host host;
    OpenMfx::EffectRegistry& registry = OpenMfx::EffectRegistry::GetInstance();
    registry.setHost(&host);
    auto library = registry.getLibrary("./OpenMfx/sdk/examples/c/plugins/OpenMfx_Example_C_Plugin_color_to_uv.ofx");

    std::cout << "Hello, I see " << library->effectCount() << " effects" << std::endl;
    std::cout << "First effect is " << library->effectIdentifier(0) << std::endl;

    auto effectDescriptor = registry.getEffectDescriptor(library, 0);

//    for (int i = 0; i < effectDescriptor->inputs.count(); i++) {
//        auto& effectInput = effectDescriptor->inputs[i];
//        std::cout << "I see input named "<< effectInput.name() <<std::endl;
//    }

    for (auto& effectInput : effectDescriptor->inputs) {
        std::cout << "I see input named "<< effectInput.name() <<std::endl;

      for (auto& prop : effectInput.properties) {
        std::cout << "I see input property named "<< prop.index() << " = " << prop.value[0].as_int <<std::endl;
      }

        for (auto& requestedAttribute : effectInput.requested_attributes) {
          std::cout << "I see requested attribute named "<< requestedAttribute.name() <<std::endl;
          for (auto& requestedAttributeProperty : requestedAttribute.properties) {
            std::cout << "I see requested attribute property named "<< requestedAttributeProperty.index() <<std::endl;
          }
        }
    }

    OfxMeshEffectHandle effectInstance;
    bool ok = host.CreateInstance(effectDescriptor, effectInstance);
    std::cout << "Created instance? " << ok <<std::endl;

    host.DestroyInstance(effectInstance);
    registry.releaseLibrary(library);
    return 0;
}
