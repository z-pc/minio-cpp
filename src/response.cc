// MinIO C++ Library for Amazon S3 Compatible Cloud Storage
// Copyright 2022 MinIO, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "response.h"

minio::s3::Response minio::s3::Response::ParseXML(std::string_view data,
                                                  int status_code,
                                                  utils::Multimap headers) {
  Response resp;
  resp.status_code = status_code;
  resp.headers = headers;

  pugi::xml_document xdoc;
  pugi::xml_parse_result result = xdoc.load_string(data.data());
  if (!result) {
    resp.err_ = error::Error("unable to parse XML; " + std::string(data));
    return resp;
  }

  auto root = xdoc.select_node("/Error");
  pugi::xpath_node text;

  text = root.node().select_node("Code/text()");
  resp.code = text.node().value();

  text = root.node().select_node("Message/text()");
  resp.message = text.node().value();

  text = root.node().select_node("Resource/text()");
  resp.resource = text.node().value();

  text = root.node().select_node("RequestId/text()");
  resp.request_id = text.node().value();

  text = root.node().select_node("HostId/text()");
  resp.host_id = text.node().value();

  text = root.node().select_node("BucketName/text()");
  resp.bucket_name = text.node().value();

  text = root.node().select_node("Key/text()");
  resp.object_name = text.node().value();

  return resp;
}

minio::s3::ListBucketsResponse minio::s3::ListBucketsResponse::ParseXML(
    std::string_view data) {
  std::list<Bucket> buckets;

  pugi::xml_document xdoc;
  pugi::xml_parse_result result = xdoc.load_string(data.data());
  if (!result) return error::Error("unable to parse XML");
  pugi::xpath_node_set xnodes =
      xdoc.select_nodes("/ListAllMyBucketsResult/Buckets/Bucket");
  for (auto xnode : xnodes) {
    std::string name;
    utils::Time creation_date;
    if (auto node = xnode.node().select_node("Name/text()").node()) {
      name = node.value();
    }
    if (auto node = xnode.node().select_node("CreationDate/text()").node()) {
      std::string value = node.value();
      creation_date = utils::Time::FromISO8601UTC(value.c_str());
    }

    buckets.push_back(Bucket{name, creation_date});
  }

  return buckets;
}

minio::s3::CompleteMultipartUploadResponse
minio::s3::CompleteMultipartUploadResponse::ParseXML(std::string_view data,
                                                     std::string version_id) {
  CompleteMultipartUploadResponse resp;

  pugi::xml_document xdoc;
  pugi::xml_parse_result result = xdoc.load_string(data.data());
  if (!result) return error::Error("unable to parse XML");

  auto root = xdoc.select_node("/CompleteMultipartUploadOutput");

  pugi::xpath_node text;

  text = root.node().select_node("Bucket/text()");
  resp.bucket_name = text.node().value();

  text = root.node().select_node("Key/text()");
  resp.object_name = text.node().value();

  text = root.node().select_node("Location/text()");
  resp.location = text.node().value();

  text = root.node().select_node("ETag/text()");
  resp.etag = utils::Trim(text.node().value(), '"');

  resp.version_id = version_id;

  return resp;
}

minio::s3::ListObjectsResponse minio::s3::ListObjectsResponse::ParseXML(
    std::string_view data, bool version) {
  ListObjectsResponse resp;

  pugi::xml_document xdoc;
  pugi::xml_parse_result result = xdoc.load_string(data.data());
  if (!result) return error::Error("unable to parse XML");

  std::string xpath = version ? "/ListVersionsResult" : "/ListBucketResult";

  auto root = xdoc.select_node(xpath.c_str());

  pugi::xpath_node text;
  std::string value;

  text = root.node().select_node("Name/text()");
  resp.name = text.node().value();

  text = root.node().select_node("EncodingType/text()");
  resp.encoding_type = text.node().value();

  text = root.node().select_node("Prefix/text()");
  value = text.node().value();
  resp.prefix = (resp.encoding_type == "url") ? curlpp::unescape(value) : value;

  text = root.node().select_node("Delimiter/text()");
  resp.delimiter = text.node().value();

  text = root.node().select_node("IsTruncated/text()");
  value = text.node().value();
  if (!value.empty()) resp.is_truncated = utils::StringToBool(value);

  text = root.node().select_node("MaxKeys/text()");
  value = text.node().value();
  if (!value.empty()) resp.max_keys = std::stoi(value);

  // ListBucketResult V1
  {
    text = root.node().select_node("Marker/text()");
    value = text.node().value();
    resp.marker =
        (resp.encoding_type == "url") ? curlpp::unescape(value) : value;

    text = root.node().select_node("NextMarker/text()");
    value = text.node().value();
    resp.next_marker =
        (resp.encoding_type == "url") ? curlpp::unescape(value) : value;
  }

  // ListBucketResult V2
  {
    text = root.node().select_node("KeyCount/text()");
    value = text.node().value();
    if (!value.empty()) resp.key_count = std::stoi(value);

    text = root.node().select_node("StartAfter/text()");
    value = text.node().value();
    resp.start_after =
        (resp.encoding_type == "url") ? curlpp::unescape(value) : value;

    text = root.node().select_node("ContinuationToken/text()");
    resp.continuation_token = text.node().value();

    text = root.node().select_node("NextContinuationToken/text()");
    resp.next_continuation_token = text.node().value();
  }

  // ListVersionsResult
  {
    text = root.node().select_node("KeyMarker/text()");
    value = text.node().value();
    resp.key_marker =
        (resp.encoding_type == "url") ? curlpp::unescape(value) : value;

    text = root.node().select_node("NextKeyMarker/text()");
    value = text.node().value();
    resp.next_key_marker =
        (resp.encoding_type == "url") ? curlpp::unescape(value) : value;

    text = root.node().select_node("VersionIdMarker/text()");
    resp.version_id_marker = text.node().value();

    text = root.node().select_node("NextVersionIdMarker/text()");
    resp.next_version_id_marker = text.node().value();
  }

  Item last_item;

  auto populate = [&resp = resp, &last_item = last_item](
                      std::list<Item>& items, pugi::xpath_node_set& contents,
                      bool is_delete_marker) -> void {
    for (auto content : contents) {
      pugi::xpath_node text;
      std::string value;
      Item item;

      text = content.node().select_node("ETag/text()");
      item.etag = utils::Trim(text.node().value(), '"');

      text = content.node().select_node("Key/text()");
      value = text.node().value();
      item.name =
          (resp.encoding_type == "url") ? curlpp::unescape(value) : value;

      text = content.node().select_node("LastModified/text()");
      value = text.node().value();
      item.last_modified = utils::Time::FromISO8601UTC(value.c_str());

      text = content.node().select_node("Owner/ID/text()");
      item.owner_id = text.node().value();

      text = content.node().select_node("Owner/DisplayName/text()");
      item.owner_name = text.node().value();

      text = content.node().select_node("Size/text()");
      value = text.node().value();
      if (!value.empty()) item.size = std::stoi(value);

      text = content.node().select_node("StorageClass/text()");
      item.storage_class = text.node().value();

      text = content.node().select_node("IsLatest/text()");
      value = text.node().value();
      if (!value.empty()) item.is_latest = utils::StringToBool(value);

      text = content.node().select_node("VersionId/text()");
      item.version_id = text.node().value();

      auto user_metadata = content.node().select_node("UserMetadata");
      for (auto metadata = user_metadata.node().first_child(); metadata;
           metadata = metadata.next_sibling()) {
        item.user_metadata[metadata.name()] = metadata.child_value();
      }

      item.is_delete_marker = is_delete_marker;

      items.push_back(item);
      last_item = item;
    }
  };

  auto contents = root.node().select_nodes(version ? "Version" : "Contents");
  populate(resp.contents, contents, false);
  // Only for ListObjectsV1.
  if (resp.is_truncated && resp.next_marker.empty()) {
    resp.next_marker = last_item.name;
  }

  auto common_prefixes = root.node().select_nodes("CommonPrefixes");
  for (auto common_prefix : common_prefixes) {
    Item item;

    text = common_prefix.node().select_node("Prefix/text()");
    value = text.node().value();
    item.name = (resp.encoding_type == "url") ? curlpp::unescape(value) : value;

    item.is_prefix = true;

    resp.contents.push_back(item);
  }

  auto delete_markers = root.node().select_nodes("DeleteMarker");
  populate(resp.contents, delete_markers, true);

  return resp;
}