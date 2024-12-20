#ifndef BASELINE_H
#define BASELINE_H

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
namespace cxbaseline
{
	class Baseline
	{
	public:
		Baseline(const std::string& name, const std::vector<std::string>& modules);
		virtual ~Baseline();

		std::string GetName() const;
		void SetName(const std::string& name);

		std::vector<std::string> GetModules() const;

		int GetSartVersion() const;

		int GetStopVersion() const;

		virtual bool Generate() = 0;

		virtual bool Compare() = 0;

		virtual bool Update() = 0;

	private:
		std::string m_name;
		std::vector<std::string> m_modules;
	};

	class BaselineFactory
	{
	public:
		BaselineFactory() = default;
		virtual ~BaselineFactory() = default;

		virtual std::string GetName() const = 0;

		virtual Baseline* Create() = 0;

		virtual void Remove(Baseline*) = 0;

		virtual bool IsOwn(Baseline*) const = 0;
	};

    template<typename T, typename std::enable_if<std::is_base_of<Baseline, T>::value> ::type* = nullptr>
	class BaselineFactoryHelper final : public BaselineFactory
    {
    public:
		BaselineFactoryHelper(const std::string& name, const std::vector<std::string>& modules)
            : m_name(name), m_modules(modules)
        {}

		virtual ~BaselineFactoryHelper()
        {}

		std::string GetName() const override
		{
			return m_name;
		}

        Baseline* Create() override
        {
            std::unique_ptr<Baseline> baseline = std::make_unique<T>(m_name, m_modules);
            Baseline* ptr = baseline.get();
            if (!ptr)
                return nullptr;

            m_baselines.emplace(ptr, std::move(baseline));
            return ptr;
        }

        void Remove(Baseline* baseline) override
        {
            if (!baseline)
                return;

            m_baselines.erase(baseline);
        }

		bool IsOwn(Baseline* baseline) const override
		{
            if (!baseline)
                return false;

			return m_baselines.count(baseline) > 0;
		}

    private:
        std::string m_name;
        std::vector<std::string> m_modules;
        std::unordered_map<Baseline*, std::unique_ptr<Baseline>> m_baselines;
    };

	enum class BaseLineType
	{
		NormalRun = 0,
		Generate,
		Compare,
		Update,
	};

	class BaseLineUtils
	{
	public:
		static bool SetRootDirectory(const std::string& dir);
		static std::string GetRootDirectory();

		static bool SetCompareDirectory(const std::string& dir);
		static std::string GetCompareDirectory();

		static void SetBaselineType(BaseLineType type);
		static BaseLineType GetBaselineType();

		static bool IsRunBaselineEnable();

		static bool RegisterBaselineFactory(BaselineFactory* factory);
		static bool RegisterBaselineFactory(BaselineFactory* factory, int startVersion);
		static void UnRegisterBaselineFactory(const std::string& name);

        static Baseline* CreateBaseline(const std::string& name);
        static Baseline* CreateBaseline(const std::string& name, int version);

		static void RemoveBaseline(Baseline*);

		static bool RunBaseline(Baseline* baseline);
	};

	template<typename T>
	class BaselineFactoryUtil
	{
	public:
		BaselineFactoryUtil(const std::string& name, const std::vector<std::string>& modules)
		{
			m_baseline_factory = std::make_unique<BaselineFactoryHelper<T>>(name, modules);
			BaseLineUtils::RegisterBaselineFactory(m_baseline_factory.get());
		}
		~BaselineFactoryUtil()
		{
			BaseLineUtils::UnRegisterBaselineFactory(m_baseline_factory->GetName());
		}
	private:
		std::unique_ptr<BaselineFactoryHelper<T>> m_baseline_factory;
	};

	template<typename T>
	class BaselineFactoryUtilEx
	{
	public:
		BaselineFactoryUtilEx(const std::string& name, const std::vector<std::string>& modules, int startVersion)
		{
			m_baseline_factory = std::make_unique<BaselineFactoryHelper<T>>(name, modules);
			BaseLineUtils::RegisterBaselineFactory(m_baseline_factory.get(), startVersion);
		}
		~BaselineFactoryUtilEx()
		{
			BaseLineUtils::UnRegisterBaselineFactory(m_baseline_factory->GetName());
		}
	private:
		std::unique_ptr<BaselineFactoryHelper<T>> m_baseline_factory;
	};
}

#define CxRegisterBaseline(BASELINE, NAME, MODULES) \
static cxbaseline::BaselineFactoryUtil<BASELINE> s_baseline_##BASELINE(NAME, MODULES);

#define CxRegisterBaselineEx(BASELINE, NAME, VERSION) \
static cxbaseline::BaselineFactoryUtilEx<BASELINE> s_baseline_##BASELINE##VERSION(NAME, VERSION);

#endif