using System;
using System.ComponentModel;
using System.Text;
using Android.BLE;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class ArduinoSample : MonoBehaviour
{
    [SerializeField] TMP_InputField m_inputField;
    [SerializeField] Button m_button;

    [SerializeField] TMP_Text m_tempatureText;
    [SerializeField] TMP_Text m_batteryText;

    [SerializeField] GameObject m_devicePanel;

    [SerializeField] BleDevice m_bleDevice;
    [SerializeField] Button m_subscribeTempBtn;
    [SerializeField] Button m_subscribeBatteryBtn;

    [SerializeField] Button m_disconnectButton;

    [SerializeField] Toggle m_ledToggle;

    [SerializeField] TMP_Text m_deviceNameText;

    readonly string genericService = "6d006143-881f-440e-a5a9-711d675c73ce";

    readonly string batteryService = "180f";
    readonly string ledCharacteristic = "19B10000-E8F2-537E-4F6C-D104768A1214";

    float m_lastScanEndTime = 0;

    void Start()
    {
        Debug.developerConsoleEnabled = true;
        Application.logMessageReceived += OnLog;
        m_button.onClick.AddListener( ScanForDevices );
        m_subscribeTempBtn.onClick.AddListener(SubscribeTemp);
        m_subscribeBatteryBtn.onClick.AddListener(SubscribeBattery);
        m_disconnectButton.onClick.AddListener(DisconnectDevice);
        m_ledToggle.onValueChanged.AddListener(OnChangeLED);
        m_devicePanel.SetActive(false);
    }



    void Update()
    {
        if (!m_button.interactable && m_lastScanEndTime<Time.time)
        {
            m_button.interactable = true;
        }
    }

    private void OnLog(string condition, string stackTrace, LogType type)
    {
        m_inputField.text += $"[{type}] {condition}\n";
    }

    public void ScanForDevices()
    {
        if (m_lastScanEndTime>Time.time)
        {
            return;
        }
        if(m_bleDevice!=null){
            m_bleDevice.Connect(OnConnected,OnDisconnected);
        }
        Debug.Log("Scanning");
        m_button.interactable = false;
        int scanDuration = 5 * 1000;
        m_lastScanEndTime = Time.time + scanDuration/1000f;
        BleManager.Instance.SearchForDevicesWithFilter(scanDuration, OnDeviceFound, serviceUuid:genericService);
        //BleManager.Instance.SearchForDevices(scanDuration, OnDeviceFound);
    }

    private void SubscribeTemp()
    {
        m_bleDevice.GetCharacteristic(genericService,"2A6E")?.Subscribe(OnTemperatureValue);
    }

    private void SubscribeBattery()
    {
        m_bleDevice.GetCharacteristic(genericService,"2a19")?.Subscribe(OnBatteryValue);
    }

    private void OnChangeLED(bool val)
    {
        byte[] bytes = new byte[1]{ (byte)(val ? 1:0) };
        Debug.Log($"Sending LED val: {BitConverter.ToString(bytes)} {val} {m_ledToggle.isOn}");
        m_bleDevice.GetCharacteristic(genericService,ledCharacteristic).Write(bytes);
    }

    private void DisconnectDevice()
    {
        m_bleDevice.Disconnect();
    }

    private void OnDeviceFound(BleDevice device)
    {
        Debug.Log($"Device Found {device.Name} {device.MacAddress}");
        foreach(var service in device.Services){
            Debug.Log($"{device.MacAddress} {service.UUID}");
        }
        device.Connect(OnConnected,OnDisconnected);
    }

    private void OnConnected(BleDevice device)
    {
        foreach(var service in device.Services){
            Debug.Log($"Service {service.UUID}");
            // if(service.UUID==BleGattService.ShortToLongUUID("1801")){
            //     service.GetCharacteristic("2A6E").Subscribe(OnTemperatureValue);
            // }
        }
        m_devicePanel.SetActive(true);
        m_bleDevice = device;
        m_deviceNameText.text = m_bleDevice.Name;
        m_bleDevice.GetCharacteristic(genericService,ledCharacteristic)?.Read(OnLedValueRetrieved);
    }

    private void OnLedValueRetrieved(bool success, byte[] data)
    {
        if(!success){
            return;
        }
        Debug.Log(BitConverter.ToString(data));
        //m_ledToggle.isOn = BitConverter.ToChar(data)!=0;
    }

    private void OnBatteryValue(byte[] data)
    {
        byte bat= data[0];
        m_batteryText.text = $"Battery {bat}%";
        
    }

    private void OnTemperatureValue(byte[] data)
    {
        var temp = BitConverter.ToInt32(data)/100f;
        m_tempatureText.text = $"Temp {temp}C";
        
    }

    private void OnDisconnected(BleDevice device)
    {
        m_devicePanel.SetActive(false);
    }
}