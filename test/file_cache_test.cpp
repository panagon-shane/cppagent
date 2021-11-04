// Ensure that gtest is the first header otherwise Windows raises an error
#include <gtest/gtest.h>
// Keep this comment to keep gtest.h above. (clang-format off/on is not working here!)

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

#include "rest_sink/file_cache.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using namespace std;
using namespace mtconnect;
using namespace mtconnect::rest_sink;

class FileCacheTest : public testing::Test
{
 protected:
  void SetUp() override
  {
    m_cache = make_unique<FileCache>();
  }

  void TearDown() override
  {
    m_cache.reset();
  }
  
  unique_ptr<FileCache> m_cache;
};

TEST_F(FileCacheTest, FindFiles)
{
  string uri("/schemas");

  // Register a file with the agent.
  auto list = m_cache->registerDirectory(uri, PROJECT_ROOT_DIR "/schemas",
                                         "1.7");
  
  ASSERT_TRUE(m_cache->hasFile("/schemas/MTConnectDevices_1.7.xsd"));
  auto file = m_cache->getFile("/schemas/MTConnectDevices_1.7.xsd");
  ASSERT_TRUE(file);
  EXPECT_EQ("text/xml", file->m_mimeType);
}

TEST_F(FileCacheTest, IconMimeType)
{
  // Register a file with the agent.
  auto list = m_cache->registerDirectory("/styles", PROJECT_ROOT_DIR "/styles",
                                         "1.7");
  
  auto file = m_cache->getFile("/styles/favicon.ico");
  ASSERT_TRUE(file);
  EXPECT_EQ("image/x-icon", file->m_mimeType);
}

TEST_F(FileCacheTest, verify_large_files_are_not_cached)
{
  // Make a cache that can only hold 1024 byte files
  m_cache = make_unique<FileCache>(1024);
  
  m_cache->addDirectory("/schemas", PROJECT_ROOT_DIR "/schemas", "none.xsd");
  m_cache->addDirectory("/styles", PROJECT_ROOT_DIR "/styles", "none.css");

  ASSERT_FALSE(m_cache->hasFile("/schemas/MTConnectDevices_1.7.xsd"));
  auto file = m_cache->getFile("/schemas/MTConnectDevices_1.7.xsd");
  ASSERT_TRUE(file);
  ASSERT_FALSE(file->m_cached);
  ASSERT_LT(0, file->m_size);
  ASSERT_TRUE(m_cache->hasFile("/schemas/MTConnectDevices_1.7.xsd"));
  
  auto css = m_cache->getFile("/styles/Streams.css");
  ASSERT_TRUE(css);
  ASSERT_TRUE(css->m_cached);
}

TEST_F(FileCacheTest, base_directory_should_redirect)
{
  m_cache->addDirectory("/schemas", PROJECT_ROOT_DIR "/schemas", "none.xsd");
  auto file = m_cache->getFile("/schemas");
  ASSERT_TRUE(file);
  ASSERT_EQ("/schemas/none.xsd", file->m_redirect);
  ASSERT_TRUE(m_cache->hasFile("/schemas"));
  ASSERT_TRUE(boost::starts_with(std::string(file->m_buffer), "<html>"));

  auto file2 = m_cache->getFile("/schemas");
  ASSERT_TRUE(file);
  ASSERT_EQ("/schemas/none.xsd", file2->m_redirect);
  ASSERT_TRUE(m_cache->hasFile("/schemas"));
  ASSERT_TRUE(boost::starts_with(std::string(file->m_buffer), "<html>"));
}

TEST_F(FileCacheTest, file_cache_should_compress_file)
{
  namespace fs = std::filesystem;
  
  // Cleanup
  fs::path zipped(PROJECT_ROOT_DIR "/test/resources");
  zipped /= "zipped_file.txt.gz";
  if (fs::exists(zipped))
  {
    fs::remove(zipped);
  }
  
  m_cache->addDirectory("/resources", PROJECT_ROOT_DIR "/test/resources", "none.txt");
  m_cache->setMinCompressedFileSize(1024);
  auto file = m_cache->getFile("/resources/zipped_file.txt");
  
  ASSERT_TRUE(file);
  EXPECT_EQ("text/plain", file->m_mimeType);
  EXPECT_TRUE(file->m_cached);
  EXPECT_FALSE(file->m_pathGz);
  
  auto gzFile = m_cache->getFile("/resources/zipped_file.txt", "gzip, deflate"s);
  
  ASSERT_TRUE(gzFile);
  EXPECT_EQ("text/plain", gzFile->m_mimeType);
  EXPECT_TRUE(gzFile->m_cached);
  EXPECT_TRUE(gzFile->m_pathGz);

  // Cleanup
  if (fs::exists(zipped))
  {
    fs::remove(zipped);
  }
}

TEST_F(FileCacheTest, file_cache_should_compress_file_async)
{
  namespace fs = std::filesystem;
  
  // Cleanup
  fs::path zipped(PROJECT_ROOT_DIR "/test/resources");
  zipped /= "zipped_file.txt.gz";
  if (fs::exists(zipped))
  {
    fs::remove(zipped);
  }
  
  m_cache->addDirectory("/resources", PROJECT_ROOT_DIR "/test/resources", "none.txt");
  m_cache->setMinCompressedFileSize(1024);

  boost::asio::io_context context;
  
  boost::asio::post(context, [&context, this]() {
    auto gzFile = m_cache->getFile("/resources/zipped_file.txt", "gzip, deflate"s, &context);
    
    ASSERT_TRUE(gzFile);
    EXPECT_EQ("text/plain", gzFile->m_mimeType);
    EXPECT_TRUE(gzFile->m_cached);
    EXPECT_TRUE(gzFile->m_pathGz);
    
    context.stop();
  });
  
  bool ran { false };
  context.post([&ran] {
    ran = true;
  });

  context.run();
  //EXPECT_TRUE(ran);

  // Cleanup
  if (fs::exists(zipped))
  {
    fs::remove(zipped);
  }
}

TEST_F(FileCacheTest, file_cache_should_recompress_if_gzip_older_than_file)
{
  namespace fs = std::filesystem;
  namespace ch = std::chrono;
  
  // Cleanup
  fs::path zipped(PROJECT_ROOT_DIR "/test/resources");
  zipped /= "zipped_file.txt.gz";
  if (fs::exists(zipped))
  {
    fs::remove(zipped);
  }
  
  m_cache->addDirectory("/resources", PROJECT_ROOT_DIR "/test/resources", "none.txt");
  m_cache->setMinCompressedFileSize(1024);
  auto gzFile = m_cache->getFile("/resources/zipped_file.txt", "gzip, deflate"s);
  
  ASSERT_TRUE(gzFile);
  EXPECT_EQ("text/plain", gzFile->m_mimeType);
  EXPECT_TRUE(gzFile->m_cached);
  EXPECT_TRUE(gzFile->m_pathGz);
  
  ASSERT_TRUE(fs::exists(*gzFile->m_pathGz));
  
  auto zipTime = fs::last_write_time(*gzFile->m_pathGz);
  auto fileTime = fs::last_write_time(gzFile->m_path);
  ASSERT_GT(zipTime, fileTime);
  
  std::this_thread::sleep_for(1s);
  auto now = ch::system_clock::to_time_t(ch::system_clock::now());
  auto fsnow2 = fs::file_time_type::clock::from_time_t(now);
  fs::last_write_time(gzFile->m_path, fsnow2);
  auto gzFile2 = m_cache->getFile("/resources/zipped_file.txt", "gzip, deflate"s);
  ASSERT_TRUE(gzFile2);

  auto zipTime2 = fs::last_write_time(*gzFile->m_pathGz);
  ASSERT_GT(zipTime2, zipTime);
  
  auto fileTime2 = fs::last_write_time(gzFile->m_path);
  ASSERT_GT(zipTime2, fileTime2);
  
  m_cache->clear();
  
  std::this_thread::sleep_for(1s);
  
  now = ch::system_clock::to_time_t(ch::system_clock::now());
  auto fsnow3 = fs::file_time_type::clock::from_time_t(now);
  fs::last_write_time(gzFile->m_path, fsnow3);
  auto gzFile3 = m_cache->getFile("/resources/zipped_file.txt", "gzip, deflate"s);
  ASSERT_TRUE(gzFile3);

  auto zipTime3 = fs::last_write_time(*gzFile->m_pathGz);
  ASSERT_GT(zipTime3, zipTime2);
  
  auto fileTime3 = fs::last_write_time(gzFile->m_path);
  ASSERT_GT(zipTime3, fileTime3);

  // Cleanup
  if (fs::exists(zipped))
  {
    fs::remove(zipped);
  }

}
