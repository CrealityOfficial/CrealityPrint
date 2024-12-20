#include "parameterinterface.h"

#include "external/kernel/kernel.h"
#include "internal/parameter/uiparametermanager.h"
#include "internal/models/uiparametertreemodel.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printprofile.h"

namespace creative_kernel {

  auto GetProcessSupportParameterKeys() -> std::set<QString> {
    decltype(GetProcessSupportParameterKeys()) set;

    [&set](const std::shared_ptr<UiParameterTreeModel>& node) {
      auto recursiver = [&set](const auto& recursiver, const auto& node) -> void {
        if (!node) {
          return;
        } else if (node->isForkNode()) {
          node->forEach([recursiver](const auto& child) {
            recursiver(recursiver, child);
          });
        } else if (node->isLeafNode()) {
          set.emplace(node->getKey());
        }
      };

      return recursiver(recursiver, node);
    }([] {
      auto process_tree = getKernel()->uiParameterManager()->getProcessTreeModel();
      auto support_tree = process_tree->getChildNode(QStringLiteral("/support"));
      return static_cast<UiParameterTreeModel*>(support_tree)->sharedFromThis();
    }());

    return set;
  }

  QString getExtruderValue(const QString& key, const QString& defaultValue)
  {
      ParameterManager* parameterManager = getKernel()->parameterManager();

      PrintMachine* machine = parameterManager->currentMachine();
      if (machine == nullptr)
      {
          return defaultValue;
      }

      ParameterDataModel* model = machine->getExtruderDataModel(0);
      if (!model)
          return defaultValue;

      ParameterDataItem* data = dynamic_cast<ParameterDataItem*>(model->getDataItem(key));
      if (data)
      {
          return data->getValue();
      }
      else
      {
          return defaultValue;
      }
  }

  QString getPrintMachineValue(const QString& key, const QString& defaultValue)
  {
      ParameterManager* parameterManager = getKernel()->parameterManager();

      PrintMachine* machine = parameterManager->currentMachine();
      if (machine == nullptr)
      {
          return defaultValue;
      }

      us::USettings* userSettings = machine->userSettings();
      if (userSettings)
      {
          QString value = userSettings->value(key, defaultValue);
          if (value != defaultValue)
              return value; 
      }

      us::USettings* settings = machine->settings();
      if (settings)
      {
          return settings->value(key, defaultValue);
      }
      else
      {
          return defaultValue;
      }
  }

  QString getPrintProfileValue(const QString& key, const QString& defaultValue)
  {
      ParameterManager* parameterManager = getKernel()->parameterManager();
      if (parameterManager->currentMachine()==nullptr)
      {
          return defaultValue;
      }
      PrintProfile* profile = dynamic_cast<PrintProfile*>(parameterManager->currentMachine()->currentProfileObject());
        if (!profile)
            return defaultValue;

        ParameterDataModel* model = dynamic_cast<ParameterDataModel*>(profile->basicDataModel());
        if (!model)
            return defaultValue;

        ParameterDataItem* data = dynamic_cast<ParameterDataItem*>(model->getDataItem(key));
        if (data)
        {
            return data->getValue();
        }
        else
        {
            return defaultValue;
        }
  }

  void setPrintProfileValue(const QString& key, const QString& value)
  {
      ParameterManager* parameterManager = getKernel()->parameterManager();
      PrintProfile* profile = dynamic_cast<PrintProfile*>(parameterManager->currentMachine()->currentProfileObject());
      if (!profile)
          return;

      ParameterDataModel* model = dynamic_cast<ParameterDataModel*>(profile->basicDataModel());
      if (!model)
          return;

      ParameterDataItem* data = dynamic_cast<ParameterDataItem*>(model->getDataItem(key));
      if (data)
      {
          data->setValue(key);
      }

  }

}  // namespace creative_kernel
