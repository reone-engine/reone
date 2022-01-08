/*
 * Copyright (c) 2020-2021 The reone project contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "../control.h"

namespace reone {

namespace gui {

constexpr int kDefaultSlotCount = 6;

class ListBox : public Control {
public:
    enum class SelectionMode {
        OnHover,
        OnClick
    };

    struct Item {
        std::string tag;
        std::string text;
        std::string iconText;
        std::shared_ptr<graphics::Texture> iconTexture;
        std::shared_ptr<graphics::Texture> iconFrame;

        std::vector<std::string> _textLines;
    };

    ListBox(
        GUI &gui,
        scene::SceneGraphs &sceneGraphs,
        graphics::Fonts &fonts,
        graphics::GraphicsContext &graphicsContext,
        graphics::Meshes &meshes,
        graphics::Pipeline &pipeline,
        graphics::Shaders &shaders,
        graphics::Textures &textures,
        graphics::Window &window,
        resource::Strings &strings) :
        Control(
            gui,
            ControlType::ListBox,
            sceneGraphs,
            fonts,
            graphicsContext,
            meshes,
            pipeline,
            shaders,
            textures,
            window,
            strings) {

        _clickable = true;
    }

    void clearItems();
    void addItem(Item &&item);
    void addTextLinesAsItems(const std::string &text);

    void clearSelection();

    void load(const resource::GffStruct &gffs) override;
    bool handleMouseMotion(int x, int y) override;
    bool handleMouseWheel(int x, int y) override;
    bool handleClick(int x, int y) override;
    void draw(const glm::ivec2 &screenSize, const glm::ivec2 &offset, const std::vector<std::string> &text) override;
    void stretch(float x, float y, int mask) override;

    void setFocus(bool focus) override;
    void setExtent(Extent extent) override;
    void setExtentHeight(int height) override;
    void setProtoItemType(ControlType type);
    void setSelectionMode(SelectionMode mode);
    void setProtoMatchContent(bool match);

    int getItemCount() const;
    const Item &getItemAt(int index) const;

    Control &protoItem() const { return *_protoItem; }
    Control &scrollBar() const { return *_scrollBar; }
    int selectedItemIndex() const { return _selectedItemIndex; }

    // Event listeners

    void setOnItemClick(std::function<void(const std::string &)> fn) { _onItemClick = std::move(fn); }

    // END Event listeners

private:
    SelectionMode _selectionMode {SelectionMode::OnHover};
    ControlType _protoItemType {ControlType::Invalid};
    std::shared_ptr<Control> _protoItem;
    std::shared_ptr<Control> _scrollBar;
    std::vector<Item> _items;
    int _slotCount {0};
    int _itemOffset {0};
    int _selectedItemIndex {-1};
    int _itemMargin {0};
    bool _protoMatchContent {false}; /**< proto item height must match its content */

    // Event listeners

    std::function<void(const std::string &)> _onItemClick;

    // END Event listeners

    void updateItemSlots();

    int getItemIndex(int y) const;
};

} // namespace gui

} // namespace reone
