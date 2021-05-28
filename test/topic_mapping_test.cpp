//
// Copyright Copyright 2009-2021, AMT – The Association For Manufacturing Technology (“AMT”)
// All rights reserved.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

// Ensure that gtest is the first header otherwise Windows raises an error
#include <gtest/gtest.h>
// Keep this comment to keep gtest.h above. (clang-format off/on is not working here!)

#include "pipeline/topic_mapper.hpp"
#include "pipeline/pipeline_context.hpp"
#include "observation/observation.hpp"
#include "device_model/device.hpp"
#include <chrono>

using namespace mtconnect;
using namespace mtconnect::pipeline;
using namespace mtconnect::observation;
using namespace mtconnect::asset;
using namespace device_model;
using namespace data_item;
using namespace std;

class MockPipelineContract : public PipelineContract
{
public:
  MockPipelineContract(std::map<string,DataItemPtr> &items,
                       std::map<string,DevicePtr> &devices)
  : m_dataItems(items), m_devices(devices)
  {
  }
  DevicePtr findDevice(const std::string &name) override {
    return m_devices[name];
  }
  DataItemPtr findDataItem(const std::string &device, const std::string &name) override
  {
    return m_dataItems[name];
  }
  void eachDataItem(EachDataItem fun) override {}
  void deliverObservation(observation::ObservationPtr obs) override {}
  void deliverAsset(AssetPtr )override {}
  void deliverAssetCommand(entity::EntityPtr ) override {}
  void deliverCommand(entity::EntityPtr )override {}
  void deliverConnectStatus(entity::EntityPtr, const StringList &, bool )override {}
  
  std::map<string,DataItemPtr> &m_dataItems;
  std::map<string,DevicePtr> &m_devices;
};

class TopicMappingTest : public testing::Test
{
protected:
  void SetUp() override
  {
    m_context = make_shared<PipelineContext>();
    m_context->m_contract = make_unique<MockPipelineContract>(m_dataItems, m_devices);
    m_mapper = make_shared<TopicMapper>(m_context, "");
    m_mapper->bind(make_shared<NullTransform>(TypeGuard<Entity>(RUN)));
  }

  void TearDown() override
  {
    m_dataItems.clear();
    m_devices.clear();
  }
  
  DataItemPtr makeDataItem(const Properties &props)
  {
    Properties ps(props);
    ErrorList errors;
    auto di = DataItem::make(ps, errors);
    m_dataItems.emplace(di->getId(), di);
    
    return di;
  }

  DevicePtr makeDevice(const std::string &name, const Properties &props)
  {
    Properties ps(props);
    ErrorList errors;
    auto d = dynamic_pointer_cast<device_model::Device>(device_model::Device::make(name, ps, errors));
    m_devices.emplace(d->getId(), d);
    
    return d;
  }

  
  shared_ptr<PipelineContext> m_context;
  shared_ptr<TopicMapper> m_mapper;
  std::map<string,DataItemPtr> m_dataItems;
  std::map<string,DevicePtr> m_devices;
};

inline DataSetEntry operator"" _E(const char *c, std::size_t)
{
  return DataSetEntry(c);
}

TEST_F(TopicMappingTest, SimpleEvent)
{
  Properties props{{"id", "a"s}, {"type", "EXECUTION"s}, {"category", "EVENT"s}};
  auto di = makeDataItem(props);
}
