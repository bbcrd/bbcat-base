
#define BBCDEBUG_LEVEL 1
#include "LoadedVersions.h"

BBC_AUDIOTOOLBOX_START

LoadedVersions::LoadedVersions()
{
}

LoadedVersions::~LoadedVersions()
{
}

/*--------------------------------------------------------------------------------*/
/** Access through singleton mechanism only
 */
/*--------------------------------------------------------------------------------*/
LoadedVersions& LoadedVersions::Get()
{
  static LoadedVersions _versions;
  return _versions;
}

/*--------------------------------------------------------------------------------*/
/** Register a library/application and its version string
 *
 * @note the return is the index of the entry which is a dummy return to allow the function to be called at load
 */
/*--------------------------------------------------------------------------------*/
uint_t LoadedVersions::Register(const char *name, const char *version)
{
  VERSION ver   = {name, version};
  uint_t  index = (uint_t)versions.size();
  versions.push_back(ver);
  return index;
}

/*--------------------------------------------------------------------------------*/
/** Return version string of loaded library/application of given name or empty string if not found
 */
/*--------------------------------------------------------------------------------*/
std::string LoadedVersions::GetVersion(const char *name)
{
  uint_t i;

  for (i = 0; i < versions.size(); i++)
  {
    if (versions[i].name == name) return versions[i].version;
  }

  return "";
}

/*--------------------------------------------------------------------------------*/
/** Return a textual list of libraries and applications and their versions
 */
/*--------------------------------------------------------------------------------*/
std::string LoadedVersions::GetVersionsList() const
{
  std::string res;
  uint_t i;

  for (i = 0; i < versions.size(); i++)
  {
    Printf(res, "%s: %s\n", versions[i].name.c_str(), versions[i].version.c_str());
  }

  return res;
}

BBC_AUDIOTOOLBOX_END