// NAPS2.WIA.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"


HRESULT CreateWiaDeviceManager(IWiaDevMgr2 **ppWiaDevMgr) //Vista or later
{
	//
	// Validate arguments
	//
	if (NULL == ppWiaDevMgr)
	{
		return E_INVALIDARG;
	}

	//
	// Initialize out variables
	//
	*ppWiaDevMgr = NULL;

	//
	// Create an instance of the device manager
	//

	//Vista or later:
	HRESULT hr = CoCreateInstance(CLSID_WiaDevMgr2, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr2, (void**)ppWiaDevMgr);

	//
	// Return the result of creating the device manager
	//
	return hr;
}

class CWiaTransferCallback : public IWiaTransferCallback
{
public: // Constructors, destructor
	CWiaTransferCallback() : m_cRef(1) {};
	~CWiaTransferCallback() {};

public: // IWiaTransferCallback
	HRESULT __stdcall TransferCallback(
		LONG                lFlags,
		WiaTransferParams   *pWiaTransferParams) override
	{
		HRESULT hr = S_OK;

		switch (pWiaTransferParams->lMessage)
		{
		case WIA_TRANSFER_MSG_STATUS:
			//...
				break;
		case WIA_TRANSFER_MSG_END_OF_STREAM:
			//...
				break;
		case WIA_TRANSFER_MSG_END_OF_TRANSFER:
			//...
				break;
		default:
			break;
		}

		return hr;
	}

	HRESULT __stdcall GetNextStream(
		LONG    lFlags,
		BSTR    bstrItemName,
		BSTR    bstrFullItemName,
		IStream **ppDestination)
	{

		HRESULT hr = S_OK;

		//  Return a new stream for this item's data.
		//
		*ppDestination = SHCreateMemStream(nullptr, 0);
		return hr;
	}

public: // IUnknown
	//... // Etc.

	HRESULT CALLBACK QueryInterface(REFIID riid, void **ppvObject) override
	{
		// Validate arguments
		if (NULL == ppvObject)
		{
			return E_INVALIDARG;
		}

		// Return the appropropriate interface
		if (IsEqualIID(riid, IID_IUnknown))
		{
			*ppvObject = static_cast<IUnknown*>(this);
		}
		else if (IsEqualIID(riid, IID_IWiaTransferCallback))
		{
			*ppvObject = static_cast<IWiaTransferCallback*>(this);
		}
		else
		{
			*ppvObject = NULL;
			return (E_NOINTERFACE);
		}

		// Increment the reference count before we return the interface
		reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
		return S_OK;
	}

	ULONG CALLBACK AddRef() override
	{
		return InterlockedIncrement((long*)&m_cRef);
	}
	ULONG CALLBACK Release() override
	{
		LONG cRef = InterlockedDecrement((long*)&m_cRef);
		if (0 == cRef)
		{
			delete this;
		}
		return cRef;
	}

private:
	/// For ref counting implementation
	LONG                m_cRef;
};

HRESULT CreateWiaDevice(IWiaDevMgr2 *pWiaDevMgr, BSTR bstrDeviceID, IWiaItem2 **ppWiaDevice)
{
	//
	// Validate arguments
	//
	if (NULL == pWiaDevMgr || NULL == bstrDeviceID || NULL == ppWiaDevice)
	{
		return E_INVALIDARG;
	}

	//
	// Initialize out variables
	//
	*ppWiaDevice = NULL;

	//
	// Create the WIA Device
	//
	HRESULT hr = pWiaDevMgr->CreateDevice(0, bstrDeviceID, ppWiaDevice);

	//
	// Return the result of creating the device
	//
	return hr;
}

HRESULT TransferWiaItem(IWiaItem2 *pIWiaItem2)
{
	// Validate arguments
	if (NULL == pIWiaItem2)
	{
		//_tprintf(TEXT("\nInvalid parameters passed"));
		return E_INVALIDARG;
	}

	// Get the IWiaTransfer interface
	IWiaTransfer *pWiaTransfer = NULL;
	HRESULT hr = pIWiaItem2->QueryInterface(IID_IWiaTransfer, (void**)&pWiaTransfer);
	if (SUCCEEDED(hr))
	{
		// Create our callback class
		CWiaTransferCallback *pWiaClassCallback = new CWiaTransferCallback;
		if (pWiaClassCallback)
		{

			LONG lItemType = 0;
			hr = pIWiaItem2->GetItemType(&lItemType);

			//download all items which have WiaItemTypeTransfer flag set
			if (lItemType & WiaItemTypeTransfer)
			{

				// If it is a folder, do folder download . Hence with one API call, all the leaf nodes of this folder 
				// will be transferred
				if ((lItemType & WiaItemTypeFolder))
				{
					//_tprintf(L"\nI am a folder item");
					hr = pWiaTransfer->Download(WIA_TRANSFER_ACQUIRE_CHILDREN, pWiaClassCallback);
					if (S_OK == hr)
					{
						//_tprintf(TEXT("\npWiaTransfer->Download() on folder item SUCCEEDED"));
					}
					else if (S_FALSE == hr)
					{
						//ReportError(TEXT("\npWiaTransfer->Download() on folder item returned S_FALSE. Folder may not be having child items"), hr);
					}
					else if (FAILED(hr))
					{
						//ReportError(TEXT("\npWiaTransfer->Download() on folder item failed"), hr);
					}
				}


				// If this is an file type, do file download
				else if (lItemType & WiaItemTypeFile)
				{
					hr = pWiaTransfer->Download(0, pWiaClassCallback);
					if (S_OK == hr)
					{
						//_tprintf(TEXT("\npWiaTransfer->Download() on file item SUCCEEDED"));
					}
					else if (S_FALSE == hr)
					{
						//ReportError(TEXT("\npWiaTransfer->Download() on file item returned S_FALSE. File may be empty"), hr);
					}
					else if (FAILED(hr))
					{
						//ReportError(TEXT("\npWiaTransfer->Download() on file item failed"), hr);
					}
				}
			}

			// Release our callback.  It should now delete itself.
			pWiaClassCallback->Release();
			pWiaClassCallback = NULL;
		}
		else
		{
			//ReportError(TEXT("\nUnable to create CWiaTransferCallback class instance"));
		}

		// Release the IWiaTransfer
		pWiaTransfer->Release();
		pWiaTransfer = NULL;
	}
	else
	{
		//ReportError(TEXT("\npIWiaItem2->QueryInterface failed on IID_IWiaTransfer"), hr);
	}
	return hr;
}

extern "C" {

	__declspec(dllexport) void __stdcall ScanTest(BSTR deviceId)
	{
		HRESULT hr;
		IWiaDevMgr2 *devMgr;
		CreateWiaDeviceManager(&devMgr);
		IEnumWIA_DEV_INFO *pWiaEnumDevInfo = NULL;
		hr = devMgr->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL, &pWiaEnumDevInfo);
		while (hr == S_OK)
		{
			IWiaPropertyStorage *pWiaPropertyStorage = NULL;
			hr = pWiaEnumDevInfo->Next(1, &pWiaPropertyStorage, NULL);
			if (hr == S_OK)
			{
				//
				// Do something with the device's IWiaPropertyStorage*
				//

				//
				// Release the device's IWiaPropertyStorage*
				//
				pWiaPropertyStorage->Release();
				pWiaPropertyStorage = NULL;
			}
		}

		if (S_FALSE == hr)
		{
			hr = S_OK;
		}

		//
		// Release the enumerator
		//
		pWiaEnumDevInfo->Release();
		pWiaEnumDevInfo = NULL;

		IWiaItem2* wiaDevice;
		CreateWiaDevice(devMgr, deviceId, &wiaDevice);

		// Maybe need to get the child?
		TransferWiaItem(wiaDevice);
	}

}