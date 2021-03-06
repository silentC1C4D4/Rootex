#include "viewport_dock.h"

#include "renderer/rendering_device.h"
#include "framework/systems/render_system.h"
#include "input/input_manager.h"
#include "ui/input_interface.h"

#include "editor/editor_system.h"
#include "editor/editor_application.h"
#include "editor/gui/inspector_dock.h"

#include "ImGuizmo.h"

ViewportDock::ViewportDock(const JSON::json& viewportJSON)
    : m_IsCameraMoving(false)
{
	m_ViewportDockSettings.m_AspectRatio = (float)viewportJSON["aspectRatio"]["x"] / (float)viewportJSON["aspectRatio"]["y"];
	m_ViewportDockSettings.m_ImageTint = EditorSystem::GetSingleton()->getColors().m_White;
	m_ViewportDockSettings.m_ImageBorderColor = EditorSystem::GetSingleton()->getColors().m_Accent;
	TextResourceFile* cameraFile = ResourceLoader::CreateTextResourceFile("editor/assets/entities/camera.entity.json");
	m_EditorCamera = EntityFactory::GetSingleton()->createEntity(cameraFile, true);
	RenderSystem::GetSingleton()->setCamera(m_EditorCamera->getComponent<CameraComponent>().get());

	TextResourceFile* gridFile = ResourceLoader::CreateTextResourceFile("editor/assets/entities/grid.entity.json");
	m_EditorGrid = EntityFactory::GetSingleton()->createEntity(gridFile, true);
}

void ViewportDock::draw(float deltaMilliseconds)
{
	if (m_ViewportDockSettings.m_IsActive)
	{
		ImGui::SetNextWindowBgAlpha(1.0f);
		if (ImGui::Begin("Viewport"))
		{
			ImVec2 region = ImGui::GetContentRegionAvail();
			if (region.x / region.y < m_ViewportDockSettings.m_AspectRatio)
			{
				region.y = region.x / m_ViewportDockSettings.m_AspectRatio;
			}
			else
			{
				region.x = region.y * m_ViewportDockSettings.m_AspectRatio;
			}

			m_ViewportDockSettings.m_ImageSize = region;

			static const ImVec2 viewportStart = ImGui::GetCursorPos();
			ImGui::Image(
			    RenderingDevice::GetSingleton()->getOffScreenRTSRVResolved().Get(),
			    m_ViewportDockSettings.m_ImageSize,
			    { 0, 0 },
			    { 1, 1 },
			    m_ViewportDockSettings.m_ImageTint,
			    m_ViewportDockSettings.m_ImageBorderColor);

			ImVec2 imageSize = ImGui::GetItemRectSize();
			ImVec2 imagePos = ImGui::GetItemRectMin();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityClass"))
				{
					const char* newEntityFile = (const char*)payload->Data;
					TextResourceFile* t = ResourceLoader::CreateTextResourceFile(newEntityFile);
					Ref<Entity> entity = EntityFactory::GetSingleton()->createEntityFromClass(t);
					if (Ref<TransformComponent> transform = entity->getComponent<TransformComponent>())
					{
						Quaternion rotation;
						Vector3 scale;
						Vector3 position;
						RenderSystem::GetSingleton()->getCamera()->getOwner()->getComponent<TransformComponent>()->getAbsoluteTransform().Decompose(scale, rotation, position);
						transform->setPosition(position);
						transform->setRotationQuaternion(rotation);
					}
					EventManager::GetSingleton()->call("OpenEntity", "EditorOpenEntity", entity);
				}
				ImGui::EndDragDropTarget();
			}

			InputInterface::s_ScaleX = Application::GetSingleton()->getWindow()->getWidth() / imageSize.x;
			InputInterface::s_ScaleY = Application::GetSingleton()->getWindow()->getHeight() / imageSize.y;
			
			InputInterface::s_Left = imagePos.x;
			InputInterface::s_Right = InputInterface::s_Left + imageSize.x;
			InputInterface::s_Top = imagePos.y;
			InputInterface::s_Bottom = InputInterface::s_Top + imageSize.y;

			static const ImVec2 viewportEnd = ImGui::GetCursorPos();

			static ImGuizmo::OPERATION gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
			static float axisSnap[3] = { 0.1f, 0.1f, 0.1f };
			static float angleSnap = { 45.0f };
			static float scaleSnap = { 0.1f };
			static float* currentSnap = nullptr;
			static ImGuizmo::MODE gizmoMode = ImGuizmo::MODE::LOCAL;
			ImGui::Begin("Viewport Tools");
			{
				ImGui::BeginGroup();
				if (ImGui::RadioButton("Translate", gizmoOperation == ImGuizmo::OPERATION::TRANSLATE))
				{
					gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Rotate", gizmoOperation == ImGuizmo::OPERATION::ROTATE))
				{
					gizmoOperation = ImGuizmo::OPERATION::ROTATE;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Scale", gizmoOperation == ImGuizmo::OPERATION::SCALE))
				{
					gizmoOperation = ImGuizmo::OPERATION::SCALE;
				}
				ImGui::EndGroup();

				if (gizmoOperation == ImGuizmo::OPERATION::TRANSLATE)
				{
					ImGui::DragFloat3("Axis Snap", axisSnap, 0.1f);
					currentSnap = axisSnap;
				}
				else if (gizmoOperation == ImGuizmo::OPERATION::ROTATE)
				{
					ImGui::DragFloat("Angle Snap", &angleSnap, 0.1f);
					currentSnap = &angleSnap;
				}
				else if (gizmoOperation == ImGuizmo::OPERATION::SCALE)
				{
					ImGui::DragFloat("Scale Snap", &scaleSnap, 0.1f);
					currentSnap = &scaleSnap;
				}

				if (ImGui::RadioButton("Local", gizmoMode == ImGuizmo::MODE::LOCAL))
				{
					gizmoMode = ImGuizmo::MODE::LOCAL;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("World", gizmoMode == ImGuizmo::MODE::WORLD))
				{
					gizmoMode = ImGuizmo::MODE::WORLD;
				}

				ImGui::DragFloat("Camera Sensitivity", &m_EditorCameraSensitivity);
				ImGui::DragFloat("Camera Speed", &m_EditorCameraSpeed);
			}
			if (ImGui::TreeNodeEx("RenderSystem"))
			{
				RenderSystem::GetSingleton()->draw();
				ImGui::TreePop();
			}
			ImGui::End();

			Matrix view = RenderSystem::GetSingleton()->getCamera()->getViewMatrix();
			Matrix proj = RenderSystem::GetSingleton()->getCamera()->getProjectionMatrix();

			Ref<Entity> openedEntity = InspectorDock::GetSingleton()->getOpenedEntity();
			if (openedEntity && openedEntity->getComponent<TransformComponent>())
			{
				ImGuizmo::SetRect(imagePos.x, imagePos.y, m_ViewportDockSettings.m_ImageSize.x, m_ViewportDockSettings.m_ImageSize.y);

				Matrix matrix = openedEntity->getComponent<TransformComponent>()->getAbsoluteTransform();
				Matrix deltaMatrix = Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);

				Ref<TransformComponent> transform = openedEntity->getComponent<TransformComponent>();
				BoundingBox boundingBox = transform->getBounds();

				struct Bounds
				{
					Vector3 m_Lower;
					Vector3 m_Higher;
				};
				Bounds bounds;
				bounds.m_Lower = Vector3(boundingBox.Center) - Vector3(boundingBox.Extents);
				bounds.m_Higher = Vector3(boundingBox.Center) + Vector3(boundingBox.Extents);

				ImGuizmo::Manipulate(
					&view.m[0][0],
					&proj.m[0][0],
					gizmoOperation,
					gizmoMode,
					&matrix.m[0][0],
					&deltaMatrix.m[0][0],
					currentSnap,
					&bounds.m_Lower.x);

				matrix *= transform->getParentAbsoluteTransform().Invert();

				transform->setTransform(matrix);
			}
			
			if (ImGui::IsWindowHovered() && InputManager::GetSingleton()->isPressed("InputSelect") && !ImGui::IsMouseDragging(ImGuiMouseButton_Left) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				Vector3 mouseFromWindow;
				{
					ImVec2 mouseFromWindowImGui;
					mouseFromWindowImGui.x = ImGui::GetMousePos().x - imagePos.x;
					mouseFromWindowImGui.y = ImGui::GetMousePos().y - imagePos.y;
					mouseFromWindow = { 2.0f * mouseFromWindowImGui.x / imageSize.x - 1.0f, (2.0f * mouseFromWindowImGui.y / imageSize.y - 1.0f) * -1.0f, 0.0f };
				}

				Vector3 origin = DirectX::XMVector3Unproject(
				    { ImGui::GetMousePos().x - imagePos.x, ImGui::GetMousePos().y - imagePos.y, 0.0f },
				    0,
				    0,
				    imageSize.x,
				    imageSize.y,
				    0,
				    1,
				    proj,
				    view,
				    Matrix::Identity);

				Vector3 dest = DirectX::XMVector3Unproject(
				    { ImGui::GetMousePos().x - imagePos.x, ImGui::GetMousePos().y - imagePos.y, 1.0f },
				    0,
				    0,
				    imageSize.x,
				    imageSize.y,
				    0,
				    1,
				    proj,
				    view,
				    Matrix::Identity);

				Vector3 direction = dest - origin;
				direction.Normalize();

				Ray ray(origin, direction);

				float minimumDistance = D3D11_FLOAT32_MAX;
				Ref<Entity> selectEntity;
				for (auto& [entityID, entity] : EntityFactory::GetSingleton()->getEntities())
				{
					if (entity->isEditorOnly())
					{
						continue;
					}

					if (Ref<TransformComponent> transform = entity->getComponent<TransformComponent>())
					{
						static float distance = 0.0f;

						BoundingBox boundingBox = transform->getBounds();
						boundingBox.Center = boundingBox.Center + transform->getAbsoluteTransform().Translation();

						boundingBox.Center.x *= transform->getScale().x;
						boundingBox.Center.y *= transform->getScale().y;
						boundingBox.Center.z *= transform->getScale().z;

						boundingBox.Extents.x *= transform->getScale().x;
						boundingBox.Extents.y *= transform->getScale().y;
						boundingBox.Extents.z *= transform->getScale().z;

						if (ray.Intersects(boundingBox, distance))
						{
							if (distance < minimumDistance && distance > 0.0f)
							{
								minimumDistance = distance;
								selectEntity = entity;
							}
						}
					}
				}

				if (selectEntity && selectEntity != openedEntity)
				{
					EventManager::GetSingleton()->call("MouseSelectEntity", "EditorOpenEntity", selectEntity);
					PRINT("Picked entity through selection: " + selectEntity->getFullName());
				}
			}

			if (ImGui::IsWindowHovered() && InputManager::GetSingleton()->isPressed("InputCameraActivate"))
			{
				static POINT cursorWhenActivated;
				if (!m_IsCameraMoving)
				{
					EditorApplication::GetSingleton()->getWindow()->showCursor(false);

					static RECT clip;
					clip.left = ImGui::GetWindowPos().x;
					clip.top = ImGui::GetWindowPos().y;
					clip.right = clip.left + ImGui::GetWindowSize().x;
					clip.bottom = clip.top + ImGui::GetWindowSize().y;

					EditorApplication::GetSingleton()->getWindow()->clipCursor(clip);

					GetCursorPos(&cursorWhenActivated);
					m_IsCameraMoving = true;
				}

				static POINT currentCursor;
				GetCursorPos(&currentCursor);

				float deltaUp = cursorWhenActivated.y - currentCursor.y;
				float deltaRight = cursorWhenActivated.x - currentCursor.x;

				m_EditorCameraPitch += deltaUp * deltaMilliseconds * 1e-3;
				m_EditorCameraYaw += deltaRight * deltaMilliseconds * 1e-3;

				SetCursorPos(cursorWhenActivated.x, cursorWhenActivated.y);

				m_EditorCamera->getComponent<TransformComponent>()->setRotation(
				    m_EditorCameraYaw * m_EditorCameraSensitivity / m_EditorCameraRotationNormalizer,
				    m_EditorCameraPitch * m_EditorCameraSensitivity / m_EditorCameraRotationNormalizer,
				    0.0f);

				m_ApplyCameraMatrix = m_EditorCamera->getComponent<TransformComponent>()->getLocalTransform();

				static const Vector3& forward = { 0.0f, 0.0f, -1.0f };
				static const Vector3& right = { 1.0f, 0.0f, 0.0f };

				if (InputManager::GetSingleton()->isPressed("InputCameraForward"))
				{
					m_ApplyCameraMatrix = Matrix::CreateTranslation(forward * m_EditorCameraSpeed * deltaMilliseconds * 1e-3) * m_ApplyCameraMatrix;
				}
				if (InputManager::GetSingleton()->isPressed("InputCameraBackward"))
				{
					m_ApplyCameraMatrix = Matrix::CreateTranslation(-forward * m_EditorCameraSpeed * deltaMilliseconds * 1e-3) * m_ApplyCameraMatrix;
				}
				if (InputManager::GetSingleton()->isPressed("InputCameraLeft"))
				{
					m_ApplyCameraMatrix = Matrix::CreateTranslation(-right * m_EditorCameraSpeed * deltaMilliseconds * 1e-3) * m_ApplyCameraMatrix;
				}
				if (InputManager::GetSingleton()->isPressed("InputCameraRight"))
				{
					m_ApplyCameraMatrix = Matrix::CreateTranslation(right * m_EditorCameraSpeed * deltaMilliseconds * 1e-3) * m_ApplyCameraMatrix;
				}
				if (InputManager::GetSingleton()->isPressed("InputCameraUp"))
				{
					m_ApplyCameraMatrix = Matrix::CreateTranslation(Vector3(0.0f, 1.0f, 0.0f) * m_EditorCameraSpeed * deltaMilliseconds * 1e-3) * m_ApplyCameraMatrix;
				}
				if (InputManager::GetSingleton()->isPressed("InputCameraDown"))
				{
					m_ApplyCameraMatrix = Matrix::CreateTranslation(Vector3(0.0f, -1.0f, 0.0f) * m_EditorCameraSpeed * deltaMilliseconds * 1e-3) * m_ApplyCameraMatrix;
				}
			}
			else
			{
				if (m_IsCameraMoving)
				{
					EditorApplication::GetSingleton()->getWindow()->showCursor(true);
					EditorApplication::GetSingleton()->getWindow()->resetClipCursor();
					m_IsCameraMoving = false;
				}
			}
			m_EditorCamera->getComponent<TransformComponent>()->setPosition(m_ApplyCameraMatrix.Translation());
		}
		ImGui::End();
	}
}
