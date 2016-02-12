/*
 * Copyright (C) 2016 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <string>
#include <vector>
#include "gazebo/common/Exception.hh"
#include "gazebo/common/Uri.hh"

using namespace gazebo;
using namespace common;

namespace gazebo
{
  namespace common
  {
    /// \internal
    /// \brief UriEntityPart private data.
    class UriEntityPrivate
    {
      /// \brief URI entity type.
      public: std::string type;

      /// \brief URI entity name.
      public: std::string name;
    };

    /// \internal
    /// \brief UriNestedEntity private data.
    class UriNestedEntityPrivate
    {
      /// \brief Nested URI entities.
      public: std::vector<UriEntity> entities;
    };

    /// \internal
    /// \brief UriParts private data.
    class UriPartsPrivate
    {
      /// \brief World name.
      public: std::string world;

      /// \brief URI nested entity.
      public: UriNestedEntity entity;

      /// \brief Parameters.
      public: std::vector<std::string> parameters;
    };

    /// \internal
    /// \brief Uri private data.
    class UriPrivate
    {
      /// \brief The individual parts of the URI.
      public: UriParts parts;

      /// \brief The string representation of the URI.
      public: std::string canonicalUri;

      /// \brief True when the object is storing a valid URI.
      public: bool correct = false;
    };
  }
}

//////////////////////////////////////////////////
UriEntity::UriEntity()
  : dataPtr(new UriEntityPrivate())
{
}

//////////////////////////////////////////////////
UriEntity::UriEntity(const UriEntity &_entity)
  : UriEntity()
{
  *this = _entity;
}

//////////////////////////////////////////////////
UriEntity::~UriEntity()
{
}

//////////////////////////////////////////////////
std::string UriEntity::Type() const
{
  return this->dataPtr->type;
}

//////////////////////////////////////////////////
std::string UriEntity::Name() const
{
  return this->dataPtr->name;
}

//////////////////////////////////////////////////
void UriEntity::SetType(const std::string &_type)
{
  this->Validate(_type);
  this->dataPtr->type = _type;
}

//////////////////////////////////////////////////
void UriEntity::SetName(const std::string &_name)
{
  this->Validate(_name);
  this->dataPtr->name = _name;
}

//////////////////////////////////////////////////
UriEntity &UriEntity::operator=(const UriEntity &_p)
{
  this->SetType(_p.Type());
  this->SetName(_p.Name());

  return *this;
}

//////////////////////////////////////////////////
void UriEntity::Validate(const std::string &_identifier)
{
  if (_identifier.find(" ") != std::string::npos)
    gzthrow("Invalid URI entity identifier (contains whitespaces)");

  if (_identifier.find("?") != std::string::npos)
    gzthrow("Invalid URI entity identifier (contains '?'");
}

//////////////////////////////////////////////////
UriNestedEntity::UriNestedEntity()
  : dataPtr(new UriNestedEntityPrivate())
{
}

//////////////////////////////////////////////////
UriNestedEntity::UriNestedEntity(const UriNestedEntity &_entity)
  : UriNestedEntity()
{
  *this = _entity;
}

//////////////////////////////////////////////////
UriNestedEntity::~UriNestedEntity()
{
}

//////////////////////////////////////////////////
UriEntity UriNestedEntity::Parent() const
{
  if (this->dataPtr->entities.empty())
    gzthrow("Empty nested entity");

  return this->dataPtr->entities.front();
}

//////////////////////////////////////////////////
UriEntity UriNestedEntity::Leaf() const
{
  if (this->dataPtr->entities.empty())
    gzthrow("Empty nested entity");

 return this->dataPtr->entities.back();
}

//////////////////////////////////////////////////
UriEntity UriNestedEntity::Entity(const unsigned int &_index) const
{
  if (_index >= this->dataPtr->entities.size())
    gzthrow("Incorrect index accessing a nested entity");

  return this->dataPtr->entities.at(_index);
}

//////////////////////////////////////////////////
unsigned int UriNestedEntity::EntityCount() const
{
  return this->dataPtr->entities.size();
}

//////////////////////////////////////////////////
void UriNestedEntity::AddEntity(const UriEntity &_entity)
{
  this->dataPtr->entities.push_back(_entity);
}

//////////////////////////////////////////////////
void UriNestedEntity::AddParentEntity(const UriEntity &_entity)
{
  this->dataPtr->entities.insert(this->dataPtr->entities.begin(), _entity);
}

//////////////////////////////////////////////////
void UriNestedEntity::Clear()
{
  this->dataPtr->entities.clear();
}

//////////////////////////////////////////////////
UriNestedEntity &UriNestedEntity::operator=(const UriNestedEntity &_p)
{
  this->dataPtr->entities.clear();
  for (auto i = 0u; i < _p.EntityCount(); ++i)
    this->AddEntity(_p.Entity(i));

  return *this;
}

//////////////////////////////////////////////////
UriParts::UriParts()
  : dataPtr(new UriPartsPrivate())
{
}

//////////////////////////////////////////////////
UriParts::UriParts(const UriParts &_parts)
  : UriParts()
{
  *this = _parts;
}

//////////////////////////////////////////////////
UriParts::~UriParts()
{
}

//////////////////////////////////////////////////
std::string UriParts::World() const
{
  return this->dataPtr->world;
}

//////////////////////////////////////////////////
UriNestedEntity &UriParts::Entity() const
{
  return this->dataPtr->entity;
}

//////////////////////////////////////////////////
std::vector<std::string> &UriParts::Parameters() const
{
  return this->dataPtr->parameters;
}

//////////////////////////////////////////////////
void UriParts::SetWorld(const std::string &_world)
{
  this->dataPtr->world = _world;
}

//////////////////////////////////////////////////
void UriParts::SetEntity(const UriNestedEntity &_entity)
{
  this->dataPtr->entity = _entity;
}

//////////////////////////////////////////////////
void UriParts::SetParameters(const std::vector<std::string> &_params)
{
  this->dataPtr->parameters = _params;
}

//////////////////////////////////////////////////
UriParts &UriParts::operator=(const UriParts &_p)
{
  this->SetWorld(_p.World());
  this->SetEntity(_p.Entity());
  this->SetParameters(_p.Parameters());

  return *this;
}

//////////////////////////////////////////////////
bool UriParts::Parse(const std::string &_uri, UriParts &_parts)
{
  size_t next;
  std::string world;
  UriNestedEntity entity;
  std::vector<std::string> params;

  if (!UriParts::ParseWorld(_uri, world, next))
    return false;

  if (!UriParts::ParseEntity(_uri, next, entity))
    return false;

  if (!UriParts::ParseParameters(_uri, next, params))
    return false;

  _parts.SetWorld(world);
  _parts.SetEntity(entity);
  _parts.SetParameters(params);

  return true;
}

//////////////////////////////////////////////////
bool UriParts::ParseWorld(const std::string &_uri, std::string &_world,
    size_t &_next)
{
  // Sanity check: Make sure that there are no white spaces.
  if (_uri.find(" ") != std::string::npos)
    return false;

  const std::string kDelimWorld = "/world/";
  auto start = _uri.find(kDelimWorld);
  if (start != 0)
    return false;

  auto from = kDelimWorld.size();
  auto to = _uri.find("/", from);
  if (to == std::string::npos)
    return false;

  _world = _uri.substr(from, to - from);
  _next = to;
  return true;
}

//////////////////////////////////////////////////
bool UriParts::ParseEntity(const std::string &_uri, size_t &_from,
    UriNestedEntity &_entity)
{
  size_t next;
  _entity.Clear();

  while (true)
  {
    UriEntity entity;
    if (!ParseOneEntity(_uri, _from, entity, next))
      return false;

    _entity.AddEntity(entity);

    _from = next;

    if ((next >= _uri.size()) || (_uri.at(next) == '?'))
      return true;

    // The URI doesn't have parameters and ends with "/".
    if ((_uri.at(next) == '/') && (next + 1 >= _uri.size()))
    {
      _from += 1;
      return true;
    }
  }

  return true;
}

//////////////////////////////////////////////////
bool UriParts::ParseOneEntity(const std::string &_uri, const size_t &_from,
    UriEntity &_entity, size_t &_next)
{
  auto next = _uri.find("/", _from + 1);
  if (next == std::string::npos)
    return false;

 _entity.SetType(_uri.substr(_from + 1, next - _from - 1));

  next += 1;
  auto to = _uri.find_first_of("/?", next);
  if (to == std::string::npos)
  {
    if (next == _uri.size())
    {
      // No name after the type.
      return false;
    }
    else
    {
      // Check whether are "?", "&", or "=" in the name as it would be invalid.
      auto name = _uri.substr(next, _uri.size() - next);
      if (name.find_first_of("?&=") != std::string::npos)
        return false;

      _entity.SetName(name);
      _next = _uri.size();
      return true;
    }
  }
  else
  {
    // Check whether are "?", "&", or "=" in the name as it would be invalid.
    auto name = _uri.substr(next, to - next);
    if (name.find_first_of("?&=") != std::string::npos)
      return false;

    _entity.SetName(name);
    _next = to;
    return true;
  }
}

//////////////////////////////////////////////////
bool UriParts::ParseParameters(const std::string &_uri, const size_t &_from,
    std::vector<std::string> &_params)
{
  size_t from = _from;
  _params.clear();

  // No parameters.
  if (_from >= _uri.size())
    return true;

  // The first character of the parameter list has to be a '?'.
  if (_uri.at(from) != '?')
    return false;

  from += 1;

  // The parameters follow this convention:
  // p=value1&p=value2
  while (true)
  {
    auto to = _uri.find("=", from);
    if ((to == std::string::npos) || (to == _uri.size() - 1))
      return false;

    auto left = _uri.substr(from, to - from);

    from = to + 1;
    std::string right;
    to = _uri.find("&", from);
    if (to == std::string::npos)
    {
      // No more parameters.
      right = _uri.substr(from);
      _params.push_back(right);
      return true;
    }
    else
    {
      right = _uri.substr(from, to - from);
    }

    _params.push_back(right);
    from = to + 1;
  }

  return true;
}

//////////////////////////////////////////////////
Uri::Uri(const std::string &_uri)
  : dataPtr(new UriPrivate())
{
  std::string uri = _uri;

  // Remove trailing '/'
  if (uri.back() == '/')
    uri.pop_back();

  if (!UriParts::Parse(uri, this->dataPtr->parts))
    gzthrow("Unable to parse URI");

  this->dataPtr->correct = true;
  this->dataPtr->canonicalUri = uri;
}

//////////////////////////////////////////////////
Uri::Uri(const Uri &_uri)
  : Uri(_uri.CanonicalUri())
{
}

//////////////////////////////////////////////////
Uri::Uri(const UriParts &_parts)
  : dataPtr(new UriPrivate())
{
  // Add the world part.
  this->dataPtr->canonicalUri = "/world/" + _parts.World();

  // Add the nested entity part.
  auto nestedEntity = _parts.Entity();
  for (auto i = 0u; i < nestedEntity.EntityCount(); ++i)
  {
    UriEntity entity = nestedEntity.Entity(i);
    this->dataPtr->canonicalUri += "/" + entity.Type() + "/" + entity.Name();
  }

  auto params = _parts.Parameters();
  if (!params.empty())
  {
    // Add the parameter part.
    this->dataPtr->canonicalUri += "?p=" + params.at(0);

    for (auto i = 1u; i < params.size(); ++i)
      this->dataPtr->canonicalUri += "&p=" + params.at(i);
  }

  if (!UriParts::Parse(this->dataPtr->canonicalUri, this->dataPtr->parts))
    gzthrow("Unable to create URI");

  this->dataPtr->correct = true;
}

//////////////////////////////////////////////////
Uri::~Uri()
{
}

//////////////////////////////////////////////////
UriParts Uri::Split() const
{
  if (!this->dataPtr->correct)
    gzthrow("This URI is malformed");

  return this->dataPtr->parts;
}

//////////////////////////////////////////////////
std::string Uri::CanonicalUri(const std::vector<std::string> &_params) const
{
  if (!this->dataPtr->correct)
    gzthrow("This URI is malformed");

  std::string result = this->dataPtr->canonicalUri;
  if (!_params.empty())
  {
    // Add the parameter part.
    result += "?p=" + _params.at(0);

    for (auto i = 1u; i < _params.size(); ++i)
      result += "&p=" + _params.at(i);
  }

  return result;
}
