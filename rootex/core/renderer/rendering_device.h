#pragma once

#include "common/common.h"

#include <d3d11.h>

#include <d3dcompiler.h>
#include <string>

#include "resource_file.h"

#include "vendor/DirectXTK/Inc/SpriteBatch.h"
#include "vendor/DirectXTK/Inc/SpriteFont.h"

/// The boss of all rendering, all DirectX API calls requiring the Device or Context go through this
class RenderingDevice
{
public:
	enum class RasterizerState
	{
		Default,
		UI,
		UIScissor,
		Wireframe,
		Sky
	};

private:
	Microsoft::WRL::ComPtr<ID3D11Device> m_Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_Context;
	
	HWND m_WindowHandle;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_MainRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_MainRTSRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_MainDSV;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_OffScreenRTTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_OffScreenRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_OffScreenRTSRV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_OffScreenRTTextureResolved;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_OffScreenRTVResolved;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_OffScreenRTSRVResolved;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_DSState;
	UINT m_StencilRef;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_SkyDSState;

	Ref<DirectX::SpriteBatch> m_FontBatch;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_DefaultRS;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_UIRS;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_UIScissoredRS;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_WireframeRS;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_SkyRS;
	ID3D11RasterizerState** m_CurrentRS;
	RasterizerState m_CurrentRSType;

	Microsoft::WRL::ComPtr<ID3D11BlendState> m_DefaultBS;
	Microsoft::WRL::ComPtr<ID3D11BlendState> m_AlphaBS;

	Microsoft::WRL::ComPtr<IDXGISwapChain> m_SwapChain;
	bool m_MSAA;
	unsigned int m_4XMSQuality;

	RenderingDevice();
	RenderingDevice(RenderingDevice&) = delete;
	~RenderingDevice();

	/// Should only be called by Window class
	void swapBuffers();

	friend class Window;
	friend class PostProcess;

public:
	static RenderingDevice* GetSingleton();
	void initialize(HWND hWnd, int width, int height, bool MSAA);
	/// Create resources which depend on window height and width
	void createSwapChainAndRTs(int width, int height, bool MSAA, const HWND& hWnd);
	void setScreenState(bool fullscreen);
	
	ID3D11Device* getDevice();
	ID3D11DeviceContext* getContext();

	void enableSkyDSS();
	void disableSkyDSS();

	void createRTVAndSRV(Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv);
	Microsoft::WRL::ComPtr<ID3D11Buffer> createVB(D3D11_BUFFER_DESC* vbd, D3D11_SUBRESOURCE_DATA* vsd, const UINT* stride, const UINT* offset);
	Microsoft::WRL::ComPtr<ID3D11Buffer> createIB(D3D11_BUFFER_DESC* ibd, D3D11_SUBRESOURCE_DATA* isd, DXGI_FORMAT format);
	Microsoft::WRL::ComPtr<ID3D11Buffer> createVSCB(D3D11_BUFFER_DESC* cbd, D3D11_SUBRESOURCE_DATA* csd);
	Microsoft::WRL::ComPtr<ID3D11Buffer> createPSCB(D3D11_BUFFER_DESC* cbd, D3D11_SUBRESOURCE_DATA* csd);
	Microsoft::WRL::ComPtr<ID3D11PixelShader> createPS(ID3DBlob* blob);
	Microsoft::WRL::ComPtr<ID3D11VertexShader> createVS(ID3DBlob* blob);
	Microsoft::WRL::ComPtr<ID3D11InputLayout> createVL(ID3DBlob* vertexShaderBlob, const D3D11_INPUT_ELEMENT_DESC* ied, UINT size);

	Ref<DirectX::SpriteFont> createFont(FileBuffer* fontFileBuffer);
	/// To hold shader blobs loaded from the compiled shader files
	Microsoft::WRL::ComPtr<ID3DBlob> createBlob(LPCWSTR path);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> createTexture(ImageResourceFile* imageRes);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> createDDSTexture(ImageResourceFile* imageRes);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> createTexture(const char* imageFileData, size_t size);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> createTextureFromPixels(const char* imageRawData, unsigned int width, unsigned int height);
	Microsoft::WRL::ComPtr<ID3D11SamplerState> createSS();

	void bind(ID3D11Buffer* vertexBuffer, const unsigned int* stride, const unsigned int* offset);
	void bind(ID3D11Buffer* indexBuffer, DXGI_FORMAT format);
	void bind(ID3D11VertexShader* vertexShader);
	void bind(ID3D11PixelShader* pixelShader);
	void bind(ID3D11InputLayout* inputLayout);

	void resolveSRV(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> source, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> destination);

	void mapBuffer(ID3D11Buffer* buffer, D3D11_MAPPED_SUBRESOURCE& subresource);
	void unmapBuffer(ID3D11Buffer* buffer);
	
	/// Binds textures used in Pixel Shader
	void setInPixelShader(unsigned int slot, unsigned int number, ID3D11ShaderResourceView* texture);
	/// Binds sampler used in sampling textures in Pixel Shader
	void setInPixelShader(ID3D11SamplerState* samplerState);
	
	void setVSCB(ID3D11Buffer* constantBuffer, UINT slot);
	void setPSCB(ID3D11Buffer* constantBuffer, UINT slot);

	void unbindShaderResources();

	void setDefaultBS();
	void setAlphaBS();
	
	void setCurrentRS();
	RasterizerState getRSType();
	void setRSType(RasterizerState rs);

	void setTemporaryUIRS();
	void setTemporaryUIScissoredRS();
	
	void setDSS();
	
	void setScissorRectangle(int x, int y, int width, int height);

	void setOffScreenRT();
	void setOffScreenRTResolved();
	void setMainRT();
	void setRTV(Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv);
	
	void unbindRTSRVs();
	void unbindRTVs();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> getMainRTSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> getOffScreenRTSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> getOffScreenRTSRVResolved();

	Ref<DirectX::SpriteBatch> getUIBatch();

	void setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY pt);
	void setViewport(const D3D11_VIEWPORT* vp);
	
	/// The last boss, draws Triangles
	void drawIndexed(UINT number);
	
	void beginDrawUI();
	void endDrawUI();

	void clearRTV(Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv, float r, float g, float b, float a);
	void clearMainRT(float r, float g, float b, float a);
	void clearOffScreenRT(float r, float g, float b, float a);
	void clearDSV();
};
