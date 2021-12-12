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

#include "dialogs.h"

#include "../../resource/gffs.h"
#include "../../resource/strings.h"

using namespace std;

using namespace reone::game;
using namespace reone::resource;

namespace reone {

namespace kotor {

shared_ptr<Dialog> Dialogs::doGet(string resRef) {
    auto dlg = _gffs.get(resRef, ResourceType::Dlg);
    if (!dlg) {
        return nullptr;
    }
    return loadDialog(*dlg);
}

unique_ptr<Dialog> Dialogs::loadDialog(const GffStruct &dlg) {
    auto dialog = make_unique<Dialog>();

    dialog->skippable = dlg.getBool("Skippable");
    dialog->cameraModel = dlg.getString("CameraModel");
    dialog->endScript = dlg.getString("EndConversation");
    dialog->animatedCutscene = dlg.getBool("AnimatedCut");
    dialog->conversationType = dlg.getEnum("ConversationType", ConversationType::Cinematic);
    dialog->computerType = dlg.getEnum("ComputerType", ComputerType::Normal);

    for (auto &entry : dlg.getList("EntryList")) {
        dialog->entries.push_back(getEntryReply(*entry));
    }
    for (auto &reply : dlg.getList("ReplyList")) {
        dialog->replies.push_back(getEntryReply(*reply));
    }
    for (auto &entry : dlg.getList("StartingList")) {
        dialog->startEntries.push_back(getEntryReplyLink(*entry));
    }
    for (auto &stunt : dlg.getList("StuntList")) {
        dialog->stunts.push_back(getStunt(*stunt));
    }

    return move(dialog);
}

Dialog::EntryReplyLink Dialogs::getEntryReplyLink(const GffStruct &gffs) const {
    Dialog::EntryReplyLink link;
    link.index = gffs.getInt("Index");
    link.active = gffs.getString("Active");

    return move(link);
}

Dialog::EntryReply Dialogs::getEntryReply(const GffStruct &gffs) const {
    int strRef = gffs.getInt("Text");

    Dialog::EntryReply entry;
    entry.speaker = gffs.getString("Speaker");
    entry.text = strRef == -1 ? "" : _strings.get(strRef);
    entry.voResRef = gffs.getString("VO_ResRef");
    entry.script = gffs.getString("Script");
    entry.sound = gffs.getString("Sound");
    entry.listener = gffs.getString("Listener");
    entry.delay = gffs.getInt("Delay");
    entry.waitFlags = gffs.getInt("WaitFlags");
    entry.cameraId = gffs.getInt("CameraID", -1);
    entry.cameraAngle = gffs.getInt("CameraAngle");
    entry.cameraAnimation = gffs.getInt("CameraAnimation", 0);
    entry.camFieldOfView = gffs.getFloat("CamFieldOfView", 0.0f);

    boost::to_lower(entry.speaker);
    boost::to_lower(entry.listener);

    for (auto &link : gffs.getList("RepliesList")) {
        entry.replies.push_back(getEntryReplyLink(*link));
    }
    for (auto &link : gffs.getList("EntriesList")) {
        entry.entries.push_back(getEntryReplyLink(*link));
    }
    for (auto &anim : gffs.getList("AnimList")) {
        entry.animations.push_back(getParticipantAnimation(*anim));
    }

    return move(entry);
}

Dialog::Stunt Dialogs::getStunt(const GffStruct &gffs) const {
    Dialog::Stunt stunt;
    stunt.participant = boost::to_lower_copy(gffs.getString("Participant"));
    stunt.stuntModel = boost::to_lower_copy(gffs.getString("StuntModel"));
    return move(stunt);
}

Dialog::ParticipantAnimation Dialogs::getParticipantAnimation(const GffStruct &gffs) const {
    Dialog::ParticipantAnimation anim;
    anim.participant = boost::to_lower_copy(gffs.getString("Participant"));
    anim.animation = gffs.getInt("Animation");
    return move(anim);
}

} // namespace kotor

} // namespace reone
