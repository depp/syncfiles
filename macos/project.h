// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef MACOS_PROJECT_H
#define MACOS_PROJECT_H

// Handle to a synchronization project.
typedef struct Project **ProjectHandle;

// Create a new empty synchronization project.
void ProjectNew(void);

// Adjust menus before selecting a menu item.
void ProjectAdjustMenus(ProjectHandle project, MenuHandle fileMenu);

// Handle a menu command or equivalent.
void ProjectCommand(WindowRef window, ProjectHandle project, int menuID,
                    int item);

// Handle an update event for a project window.
void ProjectUpdate(WindowRef window, ProjectHandle project);

// Set whether a project window is active.
void ProjectActivate(WindowRef window, ProjectHandle project, int isActive);

// Handle a mouse down event in a window.
void ProjectMouseDown(WindowRef window, ProjectHandle project,
                      EventRecord *event);

#endif
