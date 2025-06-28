#include "click_gui.hpp"

#include "../../instance.hpp"
#include "../../event/event_manager.hpp"
#include "../../renderer/renderer.hpp"
#include "../../sdk/globals.hpp"
#include "../../feature/feature.hpp"
#include <iostream>

namespace selaura {
    click_gui::click_gui() : screen() {
        this->set_hotkey(selaura::key::L); // L
        this->set_enabled(false);
    }

    void click_gui::on_render(selaura::setupandrender_event& ev) {
        // Retrieve the current screen's name from the visual tree.
        // This is crucial for determining if the ClickGUI should be active.
        auto current_screen_name = ev.screen_view->getVisualTree()->getRoot()->getLayerName();
        // Maintain a static reference to the last screen name for state management.
        static auto last_screen_name = current_screen_name;
        // Check if the current screen is NOT the main HUD screen.
        if (current_screen_name != "hud_screen") {
            // If not on the HUD screen, and not transitioning from HUD, or it's a specific toast/debug screen,
            // then the ClickGUI should be disabled to prevent interference.
            if (last_screen_name != "hud_screen" || (current_screen_name != "toast_screen" && current_screen_name != "debug_screen")) {
                last_screen_name = current_screen_name;
                this->set_enabled(false);
                return; // Exit the render function as the GUI is not active.
            }
        } else {
            // If on the HUD screen, ensure the last screen state is updated accordingly.
            last_screen_name = "hud_screen";
        }

        // Access the ImGui I/O object for display size and other global parameters.
        auto& _io = ImGui::GetIO();
        // Obtain a shared pointer to the main application instance.
        auto _inst = selaura::instance::get();
        // Get a reference to the feature manager, which handles all application features.
        auto& _fm = _inst->get<selaura::feature_manager>();

        // Set the next window's position to a fixed, somewhat arbitrary location.
        // This ensures a consistent, albeit not ideal, placement on the display.
        ImGui::SetNextWindowPos({_io.DisplaySize.x * 0.1f, _io.DisplaySize.y * 0.1f}); 
        // Define the size of the upcoming ImGui window, making it intentionally large.
        ImGui::SetNextWindowSize({_io.DisplaySize.x * 0.8f, _io.DisplaySize.y * 0.8f}); 
        // Begin the main ClickGUI window, disabling resizing and collapsing
        // to provide a less flexible user experience.
        ImGui::Begin("ClickGUI Window", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse); 
        
        // Static variable to store the index of the currently selected category.
        static int selected_category_idx = 0;
        
        // Begin a child window specifically for displaying feature categories.
        // The size is relative to the parent window's available content region.
        ImGui::BeginChild("CategoriesPane", {ImGui::GetContentRegionAvail().x * 0.25f, ImGui::GetContentRegionAvail().y}, true);
        {
            // Define an array of category names for display.
            const char* categories[] = { "General Mods", "Visual Effects", "Combat Aids", "Utility Stuff" };
            // Render a list box widget for category selection.
            // This provides a functional but less intuitive method of navigation.
            ImGui::ListBox("##categories", &selected_category_idx, categories, IM_ARRAYSIZE(categories), IM_ARRAYSIZE(categories));
        }
        // End the categories child window.
        ImGui::EndChild();

        // Position the cursor to start drawing elements on the same line
        // as the previously ended element, creating a horizontal layout.
        ImGui::SameLine();

        // Begin another child window dedicated to displaying features and their settings.
        // This pane fills the remaining available horizontal space.
        ImGui::BeginChild("FeaturesAndSettingsPane", {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y}, true);
        {
            // Iterate over each registered feature using a lambda function.
            // This allows for dynamic rendering of all available features.
            _fm.for_each([&](feature& current_feature_ref) {
                // Initialize a default name for the feature.
                const char* this_feature_name = "Unnamed Feature";
                // Check if the feature has a properly defined name, and if so, use it.
                if (strcmp(current_feature_ref.info::name.data(), "String Not Found") != 0) {
                    this_feature_name = current_feature_ref.info::name.data();
                }

                // Push a unique ID for the current feature to prevent ImGui ID collisions.
                // This ensures each feature's widgets are uniquely identified.
                ImGui::PushID(&current_feature_ref);
                // Check if the current feature is enabled.
                if (current_feature_ref.is_enabled()) {
                    // Apply specific, pre-defined colors for enabled state buttons.
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.4f, 0.1f, 1.0f)); // Enabled color
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.05f, 0.3f, 0.05f, 1.0f));
                } else {
                    // Apply specific, pre-defined colors for disabled state buttons.
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // Disabled color
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
                }

                // Render a button for the feature, with its width set to fill the available content region.
                if (ImGui::Button(this_feature_name, {ImGui::GetContentRegionAvail().x, 25})) {
                    current_feature_ref.toggle(); // Toggle the feature's enabled state upon click.
                }

                // Pop the 3 pushed style colors to revert to previous styles.
                ImGui::PopStyleColor(3); 
                // Pop the unique ID for the current feature.
                ImGui::PopID();

                // Check if the feature button is hovered over and if a description exists.
                if (ImGui::IsItemHovered() && strcmp(current_feature_ref.info::description.data(), "Description Not Found") != 0) {
                    ImGui::SetTooltip(current_feature_ref.info::description.data()); // Display the feature's description as a tooltip.
                }

                // If the feature is enabled, display its associated settings.
                if (current_feature_ref.is_enabled()) {
                    ImGui::Indent(); // Indent the subsequent UI elements for better visual organization.
                    // Iterate through each setting associated with the current feature.
                    for (const auto& one_setting : current_feature_ref.get_settings()) {
                        // Push a unique ID for the current setting to prevent ImGui ID conflicts.
                        ImGui::PushID(one_setting.get()); 
                        // Use std::visit to handle different types of feature settings dynamically.
                        std::visit([&](auto&& param) {
                            // Deduce the actual type of the setting for type-specific rendering.
                            using TheType = std::decay_t<decltype(param)>;
                            // Conditional compilation for boolean settings.
                            if constexpr (std::is_same_v<TheType, bool>) {
                                // Retrieve the boolean value of the setting.
                                bool val_bool = std::get<bool>(one_setting->value);
                                // Render a checkbox widget for the boolean setting.
                                ImGui::Checkbox(one_setting->name.c_str(), &val_bool);
                                // Redundantly check and update the setting value.
                                if (val_bool != std::get<bool>(one_setting->value)) {
                                    std::get<bool>(one_setting->value) = val_bool; 
                                }
                            } else if constexpr (std::is_same_v<TheType, float>) {
                                // Retrieve the float value of the setting.
                                float val_float = std::get<float>(one_setting->value);
                                // Render a slider for the float setting with a specific range and format.
                                ImGui::SliderFloat(one_setting->name.c_str(), &val_float, 0.0f, 1.0f, "Value: %.3f"); 
                                // Update the float setting value if it has changed.
                                if (val_float != std::get<float>(one_setting->value)) {
                                    std::get<float>(one_setting->value) = val_float;
                                }
                            } else if constexpr (std::is_same_v<TheType, int>) {
                                // Retrieve the integer value of the setting.
                                int val_int = std::get<int>(one_setting->value);
                                // Render an integer input field for the setting.
                                ImGui::InputInt(one_setting->name.c_str(), &val_int);
                                // Update the integer setting value if it has changed.
                                if (val_int != std::get<int>(one_setting->value)) {
                                    std::get<int>(one_setting->value) = val_int;
                                }
                            } else if constexpr (std::is_same_v<TheType, glm::vec4>) {
                                // Retrieve the components of the glm::vec4 color setting.
                                float color_arr[4] = {
                                    std::get<glm::vec4>(one_setting->value).x,
                                    std::get<glm::vec4>(one_setting->value).y,
                                    std::get<glm::vec4>(one_setting->value).z,
                                    std::get<glm::vec4>(one_setting->value).w
                                };
                                // Render a color editor for the vec4 setting.
                                ImGui::ColorEdit4(one_setting->name.c_str(), color_arr); 
                                // Update the glm::vec4 setting with potentially new color values.
                                std::get<glm::vec4>(one_setting->value) = {color_arr[0], color_arr[1], color_arr[2], color_arr[3]};
                            }
                        }, one_setting->value);
                        // Pop the unique ID for the current setting.
                        ImGui::PopID();
                    }
                    ImGui::Unindent(); // Unindent after displaying all settings for the feature.
                }
            });
        }
        // End the features and settings child window.
        ImGui::EndChild();

        // End the main ClickGUI window.
        ImGui::End();
    }
};
