
#include "Battery.h"
#include <winioctl.h>
#include <batclass.h>

Battery::Battery()
{
    SYSTEM_POWER_STATUS status;
    if (!GetSystemPowerStatus(&status))
    {
        throw std::system_error(std::error_code(GetLastError(),
            std::system_category()), "SYSTEM_POWER_STATUS error");
    }
    this->powerSupplyType = getPowerSupplyType(status);
    this->type = getType();
    this->batteryLifePercent = getBatteryLifePercent(status);
    this->powerSavingMode = getPowerSavingMode(status);
    this->batteryFullLifeTime = getFullBatteryLifeTime(status);
    this->batteryLifeTime = getBatteryLifeTime(status);
}

std::string Battery::getPowerSupplyType(const SYSTEM_POWER_STATUS& status)
{
    switch (status.ACLineStatus)
    {
    case 0:
        return "on battery";
    case 1:
        return "connected to power source";
    case 255:
        return "status unavialable";
    default:
        return "unknown status";
    }
}

string Battery::getType()
{
    HDEVINFO deviceInfoSet; // переменная для информации об устройствах
    // берем информацию о батарее
    deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_BATTERY, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    // информация о интерфейсе устройства
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { 0 };
    memset(&deviceInterfaceData, 0, sizeof(SP_DEVINFO_DATA));
    deviceInterfaceData.cbSize = sizeof(SP_DEVINFO_DATA);

    // получение первого интерфейса устройства, связанного с батареей
    SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &GUID_DEVCLASS_BATTERY, 0, &deviceInterfaceData);
    DWORD cbRequired = 0;

    // SetupDiGetDeviceInterfaceDetail вызывается сначала с NULL указателями для получения размера,
    // а затем с выделенными ресурсами для получения деталей интерфейса устройства.
    SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, NULL, NULL, &cbRequired, NULL);
    // выделяем память для хранения деталей интерфейса устройства.
    PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
    pdidd->cbSize = sizeof(*pdidd);

    SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, pdidd, cbRequired, &cbRequired, NULL);
    // функция CreateFile используется для открытия файла, связанного с батареей, с целью получения дескриптора файла.
    HANDLE hBattery = CreateFile(pdidd->DevicePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

    // структура запроса информации о батарее
    BATTERY_QUERY_INFORMATION batteryQueryInformation = { 0 };
    DWORD bytesWaited = 0;
    DWORD bytesReturned = 0;
    // DeviceIoControl выполняет управляющую операцию устройством (в данном случае для запроса тега батареи)
    DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_TAG, &bytesWaited, sizeof(bytesWaited), &batteryQueryInformation.BatteryTag,
        sizeof(batteryQueryInformation.BatteryTag), &bytesReturned, NULL) && batteryQueryInformation.BatteryTag;

    // структура batteryInformation для хранения информации о батарее.
    BATTERY_INFORMATION batteryInformation = { 0 };
    // устанавливаем уровень информации, который требуется получить о батарее.
    batteryQueryInformation.InformationLevel = BatteryInformation;

    // DeviceIoControl вызываем с выделенными ресурсами для получения информации о батарее.
    DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION, &batteryQueryInformation, sizeof(batteryQueryInformation),
        &batteryInformation, sizeof(batteryInformation), &bytesReturned, NULL);
    // освобождение ресурсов
    LocalFree(pdidd);
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return string(reinterpret_cast<char*>(batteryInformation.Chemistry), sizeof(batteryInformation.Chemistry));
}

string Battery::getBatteryLifePercent(const SYSTEM_POWER_STATUS& status)
{
    if (status.BatteryLifePercent == 255) return "unknown";
    return std::to_string(status.BatteryLifePercent);
}


string Battery::getFullBatteryLifeTime(const SYSTEM_POWER_STATUS& status)
{
    if (status.BatteryFullLifeTime == -1) return "unknown";
    string result = "";
    int minutes = status.BatteryFullLifeTime / 60;
    int hours = minutes / 60;
    minutes %= 60;    // убираем часы из минут
    result += std::to_string(hours) + " h " + std::to_string(minutes) + " min";
    return result;
}

string Battery::getBatteryLifeTime(const SYSTEM_POWER_STATUS& status)
{
    if (status.BatteryLifeTime == -1) return "unknown";
    string result = "";
    int minutes = status.BatteryLifeTime / 60;
    int hours = minutes / 60;
    minutes %= 60;    // убираем часы из минут
    result += std::to_string(hours) + " h " + std::to_string(minutes) + " min";
    return result;
}

string Battery::getPowerSavingMode(const SYSTEM_POWER_STATUS& status)
{
    if (status.SystemStatusFlag) return "on";
    return "off";
}
