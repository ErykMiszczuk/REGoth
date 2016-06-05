#include "WorldRender.h"
#include <engine/World.h>
#include <bgfx/bgfx.h>
#include "bgfx_utils.h"
#include "common.h"
#include "RenderSystem.h"
#include <engine/Waynet.h>
#include <debugdraw/debugdraw.h>
#include <utils/logger.h>

#include <logic/Controller.h>

namespace Render
{
	/**
	* @brief Draws the main renderpass of the given world
	*/
	void drawWorld(World::WorldInstance& world, const RenderConfig& config)
	{
		// Draw all components
		const auto& ctuple = world.getComponentDataBundle().m_Data;
		size_t num = world.getComponentDataBundle().m_NumElements;

		auto& meshes = world.getStaticMeshAllocator();
		Components::StaticMeshComponent* sms = std::get<Components::StaticMeshComponent*>(ctuple);
		Components::PositionComponent* psc = std::get<Components::PositionComponent*>(ctuple);
		Components::EntityComponent* ents = std::get<Components::EntityComponent*>(ctuple);
		Components::BBoxComponent* bboxes = std::get<Components::BBoxComponent*>(ctuple);
		Components::LogicComponent* logics = std::get<Components::LogicComponent*>(ctuple);

		size_t numIndices = 0;
		for (size_t i = 0; i<num; i++)
		{
			
			Components::ComponentMask mask = ents[i].m_ComponentMask;
			auto& mesh = meshes.getMesh(sms[i].m_StaticMeshVisual);
			auto& pos = psc[i].m_WorldMatrix;

			if ((mask & Components::StaticMeshComponent::MASK) != 0)
			{
				if (!sms[i].m_StaticMeshVisual.isValid())
					continue;

				if (mesh.m_Indices.empty())
					continue;

				if ((mask & Components::PositionComponent::MASK) != 0)
				{
					// Set model matrix for rendering.
					bgfx::setTransform(pos.m);
				}

				bgfx::setState(BGFX_STATE_DEFAULT);

				bgfx::setVertexBuffer(mesh.m_VertexBufferHandle);
				bgfx::setIndexBuffer(mesh.m_IndexBufferHandle,
					sms[i].m_SubmeshInfo.m_StartIndex,
					sms[i].m_SubmeshInfo.m_NumIndices);

				numIndices += sms[i].m_SubmeshInfo.m_NumIndices;

				if(sms[i].m_Texture.isValid())
				{
					Textures::Texture& texture = world.getTextureAllocator().getTexture(sms[i].m_Texture);
					bgfx::setTexture(0, config.uniforms.diffuseTexture, texture.m_TextureHandle);
				}

				// Submit primitive for rendering to view 0.
				bgfx::submit(0, config.programs.mainWorldProgram);
			}

			if ((mask & Components::BBoxComponent::MASK) != 0)
			{
				if(bboxes[i].m_DebugColor != 0)
				{
					Aabb box = {bboxes[i].m_BBox3D.min.x, bboxes[i].m_BBox3D.min.y, bboxes[i].m_BBox3D.min.z,
								bboxes[i].m_BBox3D.max.x, bboxes[i].m_BBox3D.max.y, bboxes[i].m_BBox3D.max.z};

					ddPush();
					Math::Matrix m = Math::Matrix::CreateIdentity();
					m.Translation(pos.Translation());
					ddSetTransform(m.mv);
					ddSetColor(bboxes[i].m_DebugColor);
					ddDraw(box);
					ddPop();
				}
			}

			if((mask & Components::LogicComponent::MASK) != 0)
			{
				if(logics[i].m_pLogicController)
				{
					logics[i].m_pLogicController->onDebugDraw();
				}
			}
		}

		bgfx::dbgTextPrintf(0, 3, 0x0f, "Num Triangles: %d", numIndices/3);


		// TODO: Debugging - remove
		/*if(!world.getWaynet().waypoints.empty())
		{
			//debugDrawWaynet(world.getWaynet());


			static std::vector<size_t> path;

			if(path.empty())
				path = World::Waynet::findWay(world.getWaynet(),
											  493,
											  342);

			debugDrawPath(world.getWaynet(), path);
		}*/
	}

	void debugDrawWaynet(const World::Waynet::WaynetInstance& waynet)
	{
		ddPush();

		ddSetTransform(nullptr);
		ddSetColor(0xFF0000FF);
		//ddSetStipple(true, 1.0f);

		const float wpAxisLen = 1.0f;
		for(const World::Waynet::Waypoint& wp : waynet.waypoints)
		{

			ddDrawAxis(wp.position.x, wp.position.y, wp.position.z, wpAxisLen);
			ddDrawAxis(wp.position.x, wp.position.y, wp.position.z, -wpAxisLen);



			// FIXME: These would be drawn twice, but I don't care right now
			for(size_t t : wp.edges)
			{
				const World::Waynet::Waypoint& target = waynet.waypoints[t];

				ddMoveTo(wp.position.x, wp.position.y, wp.position.z);
				ddLineTo(target.position.x, target.position.y, target.position.z);
			}
		}

		ddPop();
	}
}

void Render::debugDrawPath(const World::Waynet::WaynetInstance& waynet, const std::vector<size_t>& path)
{
	if(path.empty())
		return;

	ddPush();

	ddSetTransform(nullptr);
	ddSetColor(0xFF00FFFF);
	//ddSetStipple(true, 1.0f);

	const float wpAxisLen = 1.0f;

	const World::Waynet::Waypoint* start = &waynet.waypoints[path[0]];


	for(size_t t : path)
	{
		const World::Waynet::Waypoint& wp = waynet.waypoints[t];

		ddMoveTo(start->position.x, start->position.y, start->position.z);
		ddLineTo(wp.position.x, wp.position.y, wp.position.z);

		start = &wp;
	}


	ddPop();
}