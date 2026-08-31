#ifndef HEAVEN_H
#define HEAVEN_H

namespace CZ
{
    namespace HNBarAPI
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

    namespace HNCompositorAPI
    {
        struct HNIface;
        class HNCompositor;
    }

    namespace HNClientAPI
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
