#pragma once
#ifndef PathTracingEditor_H_FCB18782_BC73_4698_A684_99D9ABA5B0C4
#define PathTracingEditor_H_FCB18782_BC73_4698_A684_99D9ABA5B0C4

#include "Editor.h"

#include "Renderer/SimpleCamera.h"
#include "Renderer/SceneView.h"


namespace Render
{
	class Mesh;
}

namespace PathTracing
{
	using namespace Render;

	class RenderConfig;
	class Renderer;
	struct SceneData;

	enum class EDataActionType
	{
		ModifyRenderConfig,
		AddObject,
		RemoveObject,
		ModifyObjectProperty,
	};

	enum class EEditorCommand
	{
		SaveCamera,
		LoadCamera,
		ReloadScene,
	};

	class IEditorContext
	{
	public:
		virtual ~IEditorContext() = default;
		virtual SceneData&    getSceneData() = 0;
		virtual Renderer&     getRenderer() = 0;
		virtual RenderConfig& getRenderConfig() = 0;
		virtual SimpleCamera& getMainCamera() = 0;

		virtual Mesh& getMesh(int meshId) = 0;

		virtual void notifyDataChanged(EDataActionType type) = 0;
		virtual void executeCommand(EEditorCommand commnad) = 0;

		virtual void markViewDirty() = 0;

		virtual void loadScene(char const* path) = 0;
		virtual void saveScene(char const* path) = 0;
		virtual void addMeshFromFile(char const* path) = 0;
	};


	class PathTracingEditor : public IEditorGameViewport
	{
	public:

		IEditorContext* mContext;

		void setContext(IEditorContext* context)
		{
			mContext = context;
		}


		void initialize();
		void finalize();

		bool initializeRHI();
		void releaseRHI();


		void update(GameTimeSpan deltaTime)
		{
			mEditorCamera.updatePosition(deltaTime);
		}

		//IEditorGameViewport
		TVector2<int> getInitialSize()
		{
			return TVector2<int>(1280, 720);
		}

		void resizeViewport(int w, int h)
		{
			mEditorViewportSize = Vec2i(w, h);
		}

		void renderViewport(IEditorViewportRenderContext& context);

		void onViewportMouseEvent(MouseMsg const& msg);
		void onViewportKeyEvent(unsigned key, bool isDown);
		void onViewportCharEvent(unsigned code) override
		{

		}

		void refreshDetailView();


		void drawGizmo(RHICommandList& commandList, ViewInfo& view, Vector3 const& pos, Quaternion const& rotation);

		IEditorDetailView* mDetailView = nullptr;
		IEditorDetailView* mObjectsDetailView = nullptr;
		IEditorDetailView* mMaterialsDetailView = nullptr;
		IEditorDetailView* mMeshInfosDetailView = nullptr;
		IEditorToolBar* mToolBar = nullptr;
		SimpleCamera mEditorCamera;
		int mSelectedObjectId = INDEX_NONE;
		bool bEditorCameraValid = false;
		ViewInfo mEditorView;
		Vec2i mEditorViewportSize = Vec2i(0, 0);


		bool bNeedReload = false;
		bool bDataChanged = false;
		bool bMeshChanged = false;
		bool mbControlDown = false;


		enum class EGizmoType
		{
			None,
			Translate,
			Rotate,
			Scale,
		};
		enum class EGizmoMode
		{
			Local,
			World,
		};

		EGizmoType mGizmoType = EGizmoType::Translate;
		EGizmoMode mGizmoMode = EGizmoMode::World;
		int mGizmoAxis = -1;
		Vector3 mGizmoLastRayPos;
		Vector3 mGizmoLastRayDir;
		bool mIsGizmoDragging = false;

		RHIFrameBufferRef mFrameBuffer;
		RHIShaderResourceViewRef mStencilSRV;
		RHITexture2D* mStencilSRVTexture = nullptr;
		RHIInputLayoutRef mMeshInputLayout;

		class ScreenVS* mScreenVS;
		class EditorPreviewVS* mPreviewVS;
		class EditorPreviewPS* mPreviewPS;
		class EditorPickingPS* mPickingPS;
		class ScreenOutlinePS* mScreenOutlinePS;

		int pickObject(int x, int y);
	};




}//namespace PathTracing

#endif // PathTracingEditor_H_FCB18782_BC73_4698_A684_99D9ABA5B0C4
