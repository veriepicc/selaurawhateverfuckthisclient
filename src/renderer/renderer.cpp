﻿#include "renderer.hpp"

#include "instance.hpp"

int i = 0;

namespace selaura {

	mce::TexturePtr renderer::texturePtr;

	void renderer::set_textures_unloaded() {
		this->textures_unloaded = false;
	}

	bool renderer::initialize_imgui(MinecraftUIRenderContext& ctx) {
		ImGui::GetStyle().AntiAliasedLines = true;
		ImGui::GetStyle().AntiAliasedFill = true;

		this->load_fonts(ctx);

		return true;
	}

	void renderer::load_fonts(MinecraftUIRenderContext& ctx) {
		auto& io = ImGui::GetIO();

		unsigned char* pixels;
		int width, height, bytesPerPixel;

		//io.Fonts->AddFontFromMemoryCompressedTTF(ProductSans::compressed_data, ProductSans::compressed_size, 16.f);
		//io.Fonts->Build();

		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytesPerPixel);

		mce::Blob blob(pixels, width * height * bytesPerPixel);
		cg::ImageDescription description(width, height, mce::TextureFormat::R8G8B8A8_UNORM_SRGB, cg::ColorSpace::sRGB, cg::ImageType::Texture2D, 1);
		cg::ImageBuffer imageBuffer(std::move(blob), std::move(description));

		auto inst = selaura::instance::get();

		ResourceLocation resource("imgui_font");

		inst->get<selaura::globals>().mc_game->getTextureGroup()->uploadTexture(resource, imageBuffer);
		this->texturePtr = ctx.getTexture(resource, false);
		io.Fonts->TexID = (void*)&texturePtr;

		this->textures_unloaded = false;
	}

	void renderer::new_frame(MinecraftUIRenderContext& ctx) {
		auto& io = ImGui::GetIO();

		Vec2 screenSize = ctx.getClientInstance()->getGuiData()->getScreenSize();
		io.DisplaySize.x = screenSize.x;
		io.DisplaySize.y = screenSize.y;
	}

	void renderer::render_draw_data(ImDrawData* data, MinecraftUIRenderContext& ctx) {
		if (this->textures_unloaded) {
			load_fonts(ctx);
		}

		float scale = ctx.getClientInstance()->getGuiData()->getGuiScale();
		ScreenContext* screen_context = ctx.getScreenContext();
		Tessellator* tess = screen_context->getTessellator();

		for (int n = 0; n < data->CmdListsCount; n++) {
			ImDrawList* cmd_list = data->CmdLists[n];
			
			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
				const ImDrawCmd& cmd = cmd_list->CmdBuffer[cmd_i];
				const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
				const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

				tess->begin(mce::PrimitiveMode::TriangleList, 0);

				for (unsigned int i = 0; i < cmd.ElemCount; i += 3) {
					const ImDrawVert& v0 = vtx_buffer[idx_buffer[i + 2]];
					const ImDrawVert& v1 = vtx_buffer[idx_buffer[i + 1]];
					const ImDrawVert& v2 = vtx_buffer[idx_buffer[i + 0]];

					tess->color(v0.col);
					tess->vertexUV(v0.pos.x / scale, v0.pos.y / scale, 0.0f, v0.uv.x, v0.uv.y);

					tess->color(v1.col);
					tess->vertexUV(v1.pos.x / scale, v1.pos.y / scale, 0.0f, v1.uv.x, v1.uv.y);

					tess->color(v2.col);
					tess->vertexUV(v2.pos.x / scale, v2.pos.y / scale, 0.0f, v2.uv.x, v2.uv.y);
				}

				
				mce::MaterialPtr* material = mce::MaterialPtr::createMaterial(HashedString("ui_texture_and_color_blur"));
				MeshHelpers::renderMeshImmediately(screen_context, tess, material, *texturePtr.mClientTexture);
			}
		}
	}

	void renderer::draw_rect(glm::vec2 pos, glm::vec2 size, glm::vec4 color, float stroke_width, float radius) {
		auto drawlist = ImGui::GetBackgroundDrawList();
		drawlist->AddRect({ pos.x, pos.y }, { pos.x + size.x, pos.y + size.y }, IM_COL32(color.x, color.y, color.z, color.w), radius, 0, stroke_width);
	}
	void renderer::draw_rect(glm::vec2 pos, glm::vec2 size, glm::vec3 color, float stroke_width, float radius) {
		draw_rect(pos, size, glm::vec4(color, 1.0f), stroke_width, radius);
	}

	void renderer::draw_filled_rect(glm::vec2 pos, glm::vec2 size, glm::vec4 color, float radius, ImDrawFlags flags) {
		auto drawlist = ImGui::GetBackgroundDrawList();
		drawlist->AddRectFilled({ pos.x, pos.y }, { pos.x + size.x, pos.y + size.y }, IM_COL32(color.x, color.y, color.z, color.w), radius, flags);
	}
	void renderer::draw_filled_rect(glm::vec2 pos, glm::vec2 size, glm::vec3 color, float radius, ImDrawFlags flags) {
		draw_filled_rect(pos, size, glm::vec4(color, 1.0f), radius);
	}
}
