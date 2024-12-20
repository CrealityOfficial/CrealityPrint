#include "displaymessagemanager.h"

#include "external/data/modelgroup.h"
#include "external/data/modeln.h"
#include "external/interface/printerinterface.h"
#include "external/interface/selectorinterface.h"
#include "external/kernel/kernel.h"
#include "external/kernel/kernelui.h"
#include "external/kernel/reuseablecache.h"
#include "external/message/bedtypeinvalidmessage.h"
#include "external/message/gcodepathoutofprinterscopemessage.h"
#include "external/message/higherthanbottommessage.h"
#include "external/message/sliceenginefailmessage.h"
#include "external/message/slicewarningmessage.h"

#include "internal/models/parameterdatamodel.h"
#include "internal/multi_printer/printer.h"
#include "internal/multi_printer/printermanager.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printextruder.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printmaterial.h"

namespace creative_kernel
{
	DisplayMessageManager::DisplayMessageManager(QObject* parent)
		:QObject(parent)
	{

	}

	DisplayMessageManager::~DisplayMessageManager()
	{

	}

	bool DisplayMessageManager::checkGCodePathScope(const qtuser_3d::Box3D& gcodePathBox, Printer* printer)
	{
		if (!printer)
			return false;

		QVector3D pmin = printer->globalBox().min;
		QVector3D pmax = printer->globalBox().max;

		QVector3D pos = printer->globalBox().min;
		QMatrix4x4 printerMat;
		printerMat.setToIdentity();
		printerMat.translate(-pos);

		pmin = printerMat * pmin;
		pmax = printerMat * pmax;

		qtuser_3d::Box3D printerBox;
		printerBox += pmin;
		printerBox += pmax;

		//gcode轨迹超出平台可打印区域
		if(gcodePathBox.min.x() < printerBox.min.x() || gcodePathBox.min.y() < printerBox.min.y() ||
		   gcodePathBox.max.x() > printerBox.max.x() || gcodePathBox.max.y() > printerBox.max.y() ||
		   gcodePathBox.max.z() > printerBox.max.z() )
		{
			GCodePathOutOfPrinterScopeMessage* outofMsg = new  GCodePathOutOfPrinterScopeMessage();
			getKernelUI()->requestMessage(outofMsg);
			return true;
		}
		else
		{
			getKernelUI()->destroyMessage(MessageType::GCodePathOutofPrinterScope);
			return false;
		}
	}

	bool DisplayMessageManager::checkSliceWarning(const QMap<QString, QPair<QString, int64_t> >& warningDetails)
	{
		if (warningDetails.size() <= 0)
		{
			getKernelUI()->destroyMessage(MessageType::SliceEngineWarning);

			for (int64_t objId : m_sliceWarningRelatedObjectIds)
			{
				getKernelUI()->destroyMessage(MessageType::SliceEngineWarning + objId);
			}

			m_sliceWarningRelatedObjectIds.clear();
			return false;
		}

		SliceWarningMessage* sliceWarnMsg = new SliceWarningMessage(warningDetails);
		getKernelUI()->requestMessage(sliceWarnMsg);
		m_sliceWarningRelatedObjectIds.push_back(sliceWarnMsg->getRelatedSceneObjectId());

		return true;
	}

	void DisplayMessageManager::checkSliceEngineError(const QString& sliceErrorStr)
	{
		if (sliceErrorStr.isEmpty())
		{
			getKernelUI()->destroyMessage(MessageType::SliceEngineFail);

			for (int64_t objId : m_sliceErrorRelatedObjecIds)
			{
				getKernelUI()->destroyMessage(MessageType::SliceEngineFail + objId);
			}

			m_sliceErrorRelatedObjecIds.clear();
		}
		else
		{
			SliceEngineFailMessage* msg = new SliceEngineFailMessage(sliceErrorStr);
			getKernelUI()->requestMessage(msg);
			m_sliceErrorRelatedObjecIds.push_back(msg->getRelatedSceneObjectId());
		}
	}

	void DisplayMessageManager::checkGroupHigherThanBottom()
	{
		Printer* curPrinter = getSelectedPrinter();
		if (!curPrinter)
			return;

		QList<ModelGroup*> groups = curPrinter->modelGroupsInside();

		if (groups.size() <= 0)
		{
			getKernelUI()->destroyMessage(MessageType::ModelGroupHigherThanBottom);
			return;
		}

		bool isHigher = false;
		int groupId = -1;
		for (ModelGroup* mg : groups)
		{
			if (mg->isHigherThanBottom() && !mg->checkSupportSettingState())
			{
				isHigher = true;
				groupId = mg->getObjectId();
			}
			else
			{
				getKernelUI()->destroyMessage(MessageType::SliceEngineWarning + mg->getObjectId());
				getKernelUI()->destroyMessage(MessageType::SliceEngineFail + mg->getObjectId());
			}
		}

		if (isHigher)
		{
			HigherThanBottomMessage* hMsg = new HigherThanBottomMessage(groupId);
			getKernelUI()->requestMessage(hMsg);
		}
		else
		{
			getKernelUI()->destroyMessage(MessageType::ModelGroupHigherThanBottom);
		}

	}

	void DisplayMessageManager::updateBedTypeInvalidMessage() {
		auto* kernel = getKernelSafely();
		if (!kernel) {
			return;
		}

		auto* kernel_ui = kernel->kernelUI();
		if (!kernel_ui) {
			return;
		}

		auto* reuseable_cache = kernel->reuseableCache();
		if (!reuseable_cache) {
			return;
		}

		auto* printer_manager = reuseable_cache->getPrinterMangager();
		if (!printer_manager) {
			return;
		}

		auto* parameter_manager = kernel->parameterManager();
		if (!parameter_manager) {
			return;
		}

		int platform_index = -1;
		QString platform_bed_type{};
		std::set<int> extruder_indexes{};
		do {
			static const auto BED_TYPE_KEY = QStringLiteral("curr_bed_type");
			static const std::set<QString> POSSIBLY_INVALID_BED_TYPES{
				QStringLiteral("Epoxy Resin Plate"),
			};

			auto* selected_printer = printer_manager->getSelectedPrinter();
			if (selected_printer) {
				connect(selected_printer, &Printer::signalModelsInsideChange,
						this, &DisplayMessageManager::updateBedTypeInvalidMessage,
						Qt::ConnectionType::UniqueConnection);

				auto bed_type = selected_printer->parameter(BED_TYPE_KEY);
				if (bed_type.isEmpty()) {
					bed_type = parameter_manager->currBedType();
				}

				if (POSSIBLY_INVALID_BED_TYPES.find(bed_type) != POSSIBLY_INVALID_BED_TYPES.cend()) {
					platform_index = printer_manager->indexOfPrinter(selected_printer);
					platform_bed_type = std::move(bed_type);
					for (auto* model : selected_printer->modelsInside()) {
						connect(model, &ModelN::defaultColorIndexChanged,
								this, &DisplayMessageManager::updateBedTypeInvalidMessage,
								Qt::ConnectionType::UniqueConnection);

						connect(model, &ModelN::colorsDataChanged,
								this, &DisplayMessageManager::updateBedTypeInvalidMessage,
								Qt::ConnectionType::UniqueConnection);

						auto indexes = model->colorIndexes();
						extruder_indexes.insert(indexes.cbegin(), indexes.cend());
					}
					break;
				}
			}
		} while (false);

		if (platform_index == -1 || platform_bed_type.isEmpty()) {
			kernel_ui->destroyMessage(MessageType::BedTypeInvalid);
			return;
		}

		int material_index = -1;
		QString material_name{};
		do {
			auto* machine = parameter_manager->currentMachine();
			if (!machine) {
				break;
			}

			connect(machine, &PrintMachine::materialSaved,
					this, &DisplayMessageManager::updateBedTypeInvalidMessage,
					Qt::ConnectionType::UniqueConnection);

			for (auto extruder_index : extruder_indexes) {
				auto* extruder = machine->getExtruder(extruder_index);
				if (!extruder) {
					continue;
				}

				connect(extruder, &PrintExtruder::materialIndexChanged,
						this, &DisplayMessageManager::updateBedTypeInvalidMessage,
						Qt::ConnectionType::UniqueConnection);

				auto* material = machine->materialAt(extruder->materialIndex());
				if (!material) {
					continue;
				}

				auto* data_model = material->getDataModel();
				if (!data_model) {
					continue;
				}

				static const std::map<QString, std::set<QString>> TEMPERATURE_PARAMETERS_MAP{
					{
						QStringLiteral("High Temp Plate"), {
							QStringLiteral("hot_plate_temp"),
							QStringLiteral("hot_plate_temp_initial_layer"),
						}
					},
					{
						QStringLiteral("Textured PEI Plate"), {
							QStringLiteral("textured_plate_temp"),
							QStringLiteral("textured_plate_temp_initial_layer"),
						}
					},
					{
						QStringLiteral("Epoxy Resin Plate"), {
							QStringLiteral("epoxy_resin_plate_temp"),
							QStringLiteral("epoxy_resin_plate_temp_initial_layer"),
						}
					},
					{
						QStringLiteral("Cool Plate"), {
							QStringLiteral("cool_plate_temp"),
							QStringLiteral("cool_plate_temp_initial_layer"),
						}
					},
				};

				auto iter = TEMPERATURE_PARAMETERS_MAP.find(platform_bed_type);
				if (iter == TEMPERATURE_PARAMETERS_MAP.cend()) {
					continue;
				}

				bool found_zero_value = false;
				for (const auto& key : iter->second) {
					auto data_item = data_model->findOrMakeDataItem(key);
					connect(data_item.data(), &ParameterDataItem::valueChanged,
							this, &DisplayMessageManager::updateBedTypeInvalidMessage,
							Qt::ConnectionType::UniqueConnection);

					if (data_item->getValue() == QStringLiteral("0")) {
						found_zero_value = true;
						break;
					}
				}

				if (found_zero_value) {
					material_index = extruder_index;
					material_name = material->name();
					break;
				}
			}
		} while (false);

		if (material_index == -1 || material_name.isEmpty()) {
			kernel_ui->destroyMessage(MessageType::BedTypeInvalid);
			return;
		}

		kernel_ui->requestMessage(new BedTypeInvalidMessage{
				platform_index, platform_bed_type, material_index, material_name });
	}

	void DisplayMessageManager::onSceneChanged(const trimesh::dbox3& box)
	{
		checkGroupHigherThanBottom();
	}

	void DisplayMessageManager::onModelGroupRemoved(ModelGroup* _model_group)
	{
		if (!_model_group)
			return;

		getKernelUI()->destroyMessage(MessageType::SliceEngineWarning + _model_group->getObjectId());
		getKernelUI()->destroyMessage(MessageType::SliceEngineFail + _model_group->getObjectId());

	}

	void DisplayMessageManager::onGlobalEnableSupportChanged(bool bEnable)
	{
		checkGroupHigherThanBottom();
	}
}
