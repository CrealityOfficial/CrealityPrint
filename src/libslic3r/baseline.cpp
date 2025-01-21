#include "baseline.hpp"
#include <unordered_map>
#include <vector>

using namespace cxbaseline;

constexpr int DEFAULT_VERSION = 0;
constexpr int ERROR_VERSION = 0;

struct BaselineData
{
	BaselineFactory* factory;
	int startVersion;
};

class BaseLineUtilsPr
{
public:
	bool SetRootDirectory(const std::string& dir);
	std::string GetRootDirectory() const;

	bool SetCompareDirectory(const std::string& dir);
	std::string GetCompareDirectory()const;

	void SetBaselineType(BaseLineType type);
	BaseLineType GetBaselineType() const;

	bool RegisterBaselineFactory(BaselineFactory* factory);
	bool RegisterBaselineFactory(BaselineFactory* factory, int startVersion);
	void UnRegisterBaselineFactory(const std::string& name);

	BaselineFactory* GetFactory(const std::string& name);
	BaselineFactory* GetFactory(const std::string& name, int version);

	Baseline* CreateBaseline(const std::string& name);
	Baseline* CreateBaseline(const std::string& name, int version);
	void RemoveBaseline(Baseline* baseline);

	int GetStartVersion(const std::string& name);
	int GetStopVersion(const BaselineFactory* factory);
	int GetStopVersion(const Baseline* baseline);

private:
	BaselineData* _GetData(const std::string& name);
	BaselineData* _GetData(const std::string& name, int version);
	BaselineData* _GetData(const BaselineFactory* factory);

private:
	BaseLineType m_type = BaseLineType::NormalRun;
	std::string m_root_dir;
	std::string m_compare_dir;
	std::unordered_map<std::string, std::vector<BaselineData>> m_baselineGroup;
};

BaseLineUtilsPr& GetBaseLineUtilsPr()
{
	static BaseLineUtilsPr s_utils;
	return s_utils;
}

bool BaseLineUtilsPr::SetRootDirectory(const std::string& dir)
{
	m_root_dir = dir;
	return true;
}

std::string BaseLineUtilsPr::GetRootDirectory() const
{
	return m_root_dir;
}
bool BaseLineUtilsPr::SetCompareDirectory(const std::string& dir)
{
	m_compare_dir = dir;
	return true;
}
std::string BaseLineUtilsPr::GetCompareDirectory()const
{
	return m_compare_dir;
}

void BaseLineUtilsPr::SetBaselineType(BaseLineType type)
{
	m_type = type;
}

BaseLineType BaseLineUtilsPr::GetBaselineType() const
{
	return m_type;
}

bool BaseLineUtilsPr::RegisterBaselineFactory(BaselineFactory* factory)
{
	if (!factory)
		return false;

	std::string name = factory->GetName();
	if (name.empty())
		return false;

	if (m_baselineGroup.find(name) != m_baselineGroup.end())
		return false;

	BaselineData data;
	data.factory = factory;
	data.startVersion = DEFAULT_VERSION;

	m_baselineGroup[name].emplace_back(data);

	return true;
}
bool BaseLineUtilsPr::RegisterBaselineFactory(BaselineFactory* factory, int startVersion)
{
	if (!factory)
		return false;

	std::string name = factory->GetName();
	if (name.empty())
		return false;

	BaselineData data;
	data.factory = factory;
	data.startVersion = DEFAULT_VERSION;

	auto iter = m_baselineGroup.find(name);
	if (iter != m_baselineGroup.end())
	{
		if (m_baselineGroup[name].back().startVersion >= startVersion)
			return false;
	}

	m_baselineGroup[name].emplace_back(data);

	return true;
}
void BaseLineUtilsPr::UnRegisterBaselineFactory(const std::string& name)
{
	m_baselineGroup.erase(name);
}

BaselineFactory* BaseLineUtilsPr::GetFactory(const std::string& name)
{
	BaselineData* data = _GetData(name);
	if (!data)
		return nullptr;

	return data->factory;
}
BaselineFactory* BaseLineUtilsPr::GetFactory(const std::string& name, int version)
{
	BaselineData* data = _GetData(name, version);
	if (!data)
		return nullptr;

	return data->factory;
}

Baseline* BaseLineUtilsPr::CreateBaseline(const std::string& name)
{
    BaselineFactory* factory = GetFactory(name);
    if (!factory)
        return nullptr;
    return factory->Create();
}

Baseline* BaseLineUtilsPr::CreateBaseline(const std::string& name, int version)
{
    BaselineFactory* factory = GetFactory(name, version);
    if (!factory)
        return nullptr;
    return factory->Create();
}

void BaseLineUtilsPr::RemoveBaseline(Baseline* baseline)
{
	if (!baseline)
		return;

    auto iter = m_baselineGroup.find(baseline->GetName());
    if (iter == m_baselineGroup.end())
        return;

    std::vector<BaselineData>& data = iter->second;
    for (int i = 0; i < data.size(); ++i)
    {
		if (data[i].factory->IsOwn(baseline))
		{
			data[i].factory->Remove(baseline);
		}
    }
}

int BaseLineUtilsPr::GetStartVersion(const std::string& name)
{
	BaselineData* data = _GetData(name);
	if (!data)
		return ERROR_VERSION;

	return data->startVersion;
}

int BaseLineUtilsPr::GetStopVersion(const BaselineFactory* factory)
{
	if (!factory)
		return ERROR_VERSION;

	auto iter = m_baselineGroup.find(factory->GetName());
	if (iter == m_baselineGroup.end())
		return ERROR_VERSION;

	int index = -1;
	std::vector<BaselineData>& data = iter->second;
	for (int i = 0; i < data.size(); ++i)
	{
		if (data[i].factory == factory)
		{
			index = i;
			break;
		}
	}
	if (0 <= index && index < data.size()-1)
	{
		return data[index + 1].startVersion;
	}
	else
	{
		return ERROR_VERSION;
	}
}

int BaseLineUtilsPr::GetStopVersion(const Baseline* baseline)
{
    if (!baseline)
        return ERROR_VERSION;

	return GetStopVersion(GetFactory(baseline->GetName()));
}

BaselineData* BaseLineUtilsPr::_GetData(const std::string& name)
{
	auto iter = m_baselineGroup.find(name);
	if (iter == m_baselineGroup.end())
		return nullptr;

	return &(iter->second.front());
}

BaselineData* BaseLineUtilsPr::_GetData(const std::string& name, int version)
{
	if (version < DEFAULT_VERSION)
		return nullptr;

	auto iter = m_baselineGroup.find(name);
	if (iter == m_baselineGroup.end())
		return nullptr;

	BaselineData* data = nullptr;
	for (auto& value : iter->second)
	{
		if (value.startVersion <= version)
		{
			data = &value;
		}
		else
		{
			break;
		}
	}

	return data;
}

BaselineData* BaseLineUtilsPr::_GetData(const BaselineFactory* factory)
{
	if (!factory)
		return nullptr;

	auto iter = m_baselineGroup.find(factory->GetName());
	if (iter == m_baselineGroup.end())
		return nullptr;

	BaselineData* data = nullptr;
	for (auto& value : iter->second)
	{
		if (value.factory == factory)
		{
			data = &value;
			break;
		}
	}

	return data;
}

Baseline::Baseline(const std::string& name, const std::vector<std::string>& modules)
	:m_name(name), m_modules(modules)
{

}

Baseline::~Baseline()
{

}

std::string Baseline::GetName() const
{
	return m_name;
}
void Baseline::SetName(const std::string& name)
{
	m_name = name;
}
std::vector<std::string> Baseline::GetModules() const
{
	return m_modules;
}

int Baseline::GetSartVersion() const
{
	return GetBaseLineUtilsPr().GetStartVersion(this->GetName());
}

int Baseline::GetStopVersion() const
{
	return GetBaseLineUtilsPr().GetStopVersion(this);
}

bool BaseLineUtils::SetRootDirectory(const std::string& dir)
{
	return GetBaseLineUtilsPr().SetRootDirectory(dir);
}

std::string BaseLineUtils::GetRootDirectory()
{
	return GetBaseLineUtilsPr().GetRootDirectory();
}

bool  BaseLineUtils::SetCompareDirectory(const std::string& dir)
{
	return GetBaseLineUtilsPr().SetCompareDirectory(dir);
}
std::string BaseLineUtils::GetCompareDirectory()
{
	return GetBaseLineUtilsPr().GetCompareDirectory();
}

void BaseLineUtils::SetBaselineType(BaseLineType type)
{
	GetBaseLineUtilsPr().SetBaselineType(type);
}

BaseLineType BaseLineUtils::GetBaselineType()
{
	return GetBaseLineUtilsPr().GetBaselineType();
}

bool BaseLineUtils::IsRunBaselineEnable()
{
	return GetBaselineType() != BaseLineType::NormalRun;
}

bool BaseLineUtils::RegisterBaselineFactory(BaselineFactory* factory)
{
	return GetBaseLineUtilsPr().RegisterBaselineFactory(factory);
}
bool BaseLineUtils::RegisterBaselineFactory(BaselineFactory* factory, int startVersion)
{
	return GetBaseLineUtilsPr().RegisterBaselineFactory(factory, startVersion);
}
void BaseLineUtils::UnRegisterBaselineFactory(const std::string& name)
{
	GetBaseLineUtilsPr().UnRegisterBaselineFactory(name);
}

Baseline* BaseLineUtils::CreateBaseline(const std::string& name)
{
	return GetBaseLineUtilsPr().CreateBaseline(name);
}
Baseline* BaseLineUtils::CreateBaseline(const std::string& name, int version)
{
	return GetBaseLineUtilsPr().CreateBaseline(name, version);
}

void BaseLineUtils::RemoveBaseline(Baseline* baseline)
{
	GetBaseLineUtilsPr().RemoveBaseline(baseline);
}

bool BaseLineUtils::RunBaseline(Baseline* baseline)
{
	if (!baseline)
		return false;
	bool ret = true;
	switch (GetBaselineType())
	{
	case BaseLineType::NormalRun:
	{
		ret = true;
		break;
	}
	case BaseLineType::Generate:
	{
		ret = baseline->Generate();
		break;
	}
	case BaseLineType::Compare:
	{
		ret = baseline->Compare();
		break;
	}
	case BaseLineType::Update:
	{
		ret = baseline->Update();
		break;
	}
	default:
		ret = true;
		break;
	}
	return ret;
}