#ifndef HEAVEN_H
#define HEAVEN_H

namespace CZ
{
    namespace Bar
    {
        struct HNIface;
        struct HNEvent;
        class HNBar;
        class HNClient;
        class HNCompositor;
        class HNObject;
        class HNTopbar;
        class HNMenu;
        class HNAction;
        class HNToggle;
        class HNDivider;

        class HNWithTitle;
        class HNWithIcon;
        class HNWithParent;
        class HNWithChildren;
        class HNWithShortcut;
        class HNWithEnabled;
    }

    namespace Compositor
    {
        struct HNIface;
        class HNCompositor;
    }

    namespace Client
    {
        struct HNIface;
        class HNClient;
        class HNObject;
        class HNTopbar;
        class HNMenu;
        class HNAction;
        class HNToggle;
        class HNDivider;

        class HNWithTitle;
        class HNWithIcon;
        class HNWithParent;
        class HNWithChildren;
        class HNWithShortcut;
        class HNWithEnabled;
    }
};

#endif // HEAVEN_H
