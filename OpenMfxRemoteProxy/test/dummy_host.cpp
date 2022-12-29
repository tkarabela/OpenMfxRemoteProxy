#include <iostream>
#include "OpenMfx/Sdk/Cpp/Host/Host"
#include "OpenMfx/Sdk/Cpp/Host/EffectRegistry"
#include "OpenMfx/Sdk/Cpp/Host/EffectLibrary"
#include "OpenMfx/Sdk/Cpp/Host/MeshEffect"
#include "OpenMfx/Sdk/Cpp/Host/MeshProps"
#include "OpenMfx/Sdk/Cpp/Host/Mesh"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: dummy_host remote_proxy_plugin.ofx\n";
        return 1;
    }

    auto ofx_filepath = argv[1];

    OpenMfx::Host host;
    OpenMfx::EffectRegistry& registry = OpenMfx::EffectRegistry::GetInstance();
    registry.setHost(&host);
    auto library = registry.getLibrary(ofx_filepath);

    std::cout << "Hello, I see " << library->effectCount() << " effects" << std::endl;
    std::cout << "First effect is " << library->effectIdentifier(0) << std::endl;

    auto effectDescriptor = registry.getEffectDescriptor(library, 0);

    for (auto& effectInput : effectDescriptor->inputs) {
        std::cout << "I see input named "<< effectInput.name() <<std::endl;

      for (auto& prop : effectInput.properties) {
        std::cout << "I see input property named "<< prop.index() << " = ";
        switch (prop.type) {
          case OpenMfx::PropertyType::Int:
            std::cout << prop.value[0].as_int << " (int)";
            break;
          case OpenMfx::PropertyType::Double:
            std::cout << prop.value[0].as_double<< " (double)";
            break;
          case OpenMfx::PropertyType::String:
            std::cout << prop.value[0].as_const_char<< " (string)";
            break;
          case OpenMfx::PropertyType::Pointer:
            std::cout << prop.value[0].as_pointer<< " (pointer)";
            break;
          default:
            std::cout << prop.value[0].as_int<< " (UNKNOWN TYPE!)";
            break;
        }
        std::cout << std::endl;
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
