#include <iostream>
#include <fstream>
#include "OpenMfx/Sdk/Cpp/Host/Host"
#include "OpenMfx/Sdk/Cpp/Host/EffectRegistry"
#include "OpenMfx/Sdk/Cpp/Host/EffectLibrary"
#include "OpenMfx/Sdk/Cpp/Host/MeshEffect"
#include "OpenMfx/Sdk/Cpp/Host/MeshProps"
#include "OpenMfx/Sdk/Cpp/Host/Mesh"
#include "messages_generated.h"
#include <zmq.hpp>

int main() {
  OpenMfx::Host host;
    OpenMfx::EffectRegistry& registry = OpenMfx::EffectRegistry::GetInstance();
    registry.setHost(&host);
    auto library = registry.getLibrary("./cmake-build-debug/OpenMfx/sdk/examples/c/plugins/OpenMfx_Example_C_Plugin_color_to_uv.ofx");

    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::push);
    sock.bind("inproc://test");
    sock.send(zmq::str_buffer("Hello, world"), zmq::send_flags::dontwait);

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

  // flatbuffers
  flatbuffers::FlatBufferBuilder builder(1024);
    auto plugin_name = builder.CreateString(library->effectIdentifier(0));
  std::vector<flatbuffers::Offset<OpenMfxRemoteProxy::OfxMesh>> inputs;

  for (auto& effectInput : effectDescriptor->inputs) {
    std::vector<flatbuffers::Offset<OpenMfxRemoteProxy::OfxProperty>> input_properties;

    for (auto& prop : effectInput.properties) {
      auto property_name = builder.CreateString(prop.index());

      if (prop.type == OpenMfx::PropertyType::Int) {
        auto values = builder.CreateVector(&(prop.value[0].as_int), 1); // TODO determine size
        auto property_value = OpenMfxRemoteProxy::CreateOfxPropertyIntNValue(builder, values);
        input_properties.push_back(OpenMfxRemoteProxy::CreateOfxProperty(builder, property_name,
                                                                         OpenMfxRemoteProxy::OfxPropertyValue_OfxPropertyIntNValue,
                                                                         property_value.Union()));
      } else if (prop.type == OpenMfx::PropertyType::String) {
        auto values = builder.CreateVectorOfStrings(&(prop.value[0].as_const_char), &(prop.value[0].as_const_char)+1); // TODO determine size
        auto property_value = OpenMfxRemoteProxy::CreateOfxPropertyStringNValue(builder, values);
        input_properties.push_back(OpenMfxRemoteProxy::CreateOfxProperty(builder, property_name,
                                                                         OpenMfxRemoteProxy::OfxPropertyValue_OfxPropertyStringNValue,
                                                                         property_value.Union()));
      } else {
        std::cout <<"XXX not writing to flatbuffer a property" <<std::endl;
      }
    }

    auto properties_vec = builder.CreateVector(input_properties);
    auto mesh = OpenMfxRemoteProxy::CreateOfxMesh(builder, properties_vec);
    inputs.push_back(mesh);
  }

  auto inputs_vec = builder.CreateVector(inputs);
  auto describe = OpenMfxRemoteProxy::CreateOpenMfxRemoteProxyDescribe(builder, plugin_name, inputs_vec);

  std::cout <<"finishing flatbuffer builder" <<std::endl;
  builder.Finish(describe);
  // This must be called after `Finish()`.
  uint8_t *buf = builder.GetBufferPointer();
  auto bufsize = builder.GetSize(); // Returns the size of the buffer that
  // `GetBufferPointer()` points to.
  std::cout <<"writing flatbuffer to file" <<std::endl;
  std::ofstream outfile;
  outfile.open("temp/describe_test.bin", std::ios::binary | std::ios::out);
  if (!outfile.good()) {
    std::cout <<"bad outfile!" <<std::endl;

  }

  outfile.write(reinterpret_cast<const char *>(buf), bufsize);
  outfile.close();

  OfxMeshEffectHandle effectInstance;
    bool ok = host.CreateInstance(effectDescriptor, effectInstance);
    std::cout << "Created instance? " << ok <<std::endl;

    host.DestroyInstance(effectInstance);
    registry.releaseLibrary(library);
    return 0;
}
