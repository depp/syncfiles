// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef MACOS_PROJECT_H
#define MACOS_PROJECT_H

// A ProjectHandle is a handle to a synchronization project.
typedef struct Project **ProjectHandle;

// ProjectNew creates a new empty synchronization project.
void ProjectNew(void);

// ProjectOpen opens an existing synchronization project.
void ProjectOpen(FSSpec *spec, ScriptCode script);

// ProjectAdjustMenus adjusts menus before selecting a menu item.
void ProjectAdjustMenus(ProjectHandle project, MenuHandle fileMenu,
                        MenuHandle projectMenu);

// ProjectCommand handles a menu command or equivalent.
void ProjectCommand(WindowRef window, ProjectHandle project, int menuID,
                    int item);

// ProjectUpdate handles an update event for a project window.
void ProjectUpdate(WindowRef window, ProjectHandle project);

// ProjectActivate sets whether a project window is active.
void ProjectActivate(WindowRef window, ProjectHandle project, int isActive);

// ProjectMouseDown handles a mouse down event in a window.
void ProjectMouseDown(WindowRef window, ProjectHandle project,
                      EventRecord *event);

#endif
