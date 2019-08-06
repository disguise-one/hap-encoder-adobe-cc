#include "../ui.h"

#include "codec_registration.hpp"
#import <Cocoa/Cocoa.h>
#include <vector>
#include "FNDControlViewController.h"

#if !__has_feature(objc_arc)
#error File requires compilation with ARC
#endif
#ifndef FOUNDATION_MACOSX_BUNDLE_GUI_IDENTIFIER
#error FOUNDATION_MACOSX_BUNDLE_GUI_IDENTIFIER must be defined
#endif

@class FND_OBJC(AEXViewController);
@compatibility_alias FNDAEXViewController FND_OBJC(AEXViewController);
@interface FNDAEXViewController : NSViewController <FNDControlDelegate>
-(IBAction)ok:(id)sender;
-(IBAction)cancel:(id)sender;
@property (weak) IBOutlet NSStackView *stackView;
@property void(^changeHandler)(FNDAEXViewController *view, FNDControlViewController *control);
@end
@implementation FNDAEXViewController

-(IBAction)ok:(id)sender
{
    [[NSApplication sharedApplication] stopModalWithCode:NSModalResponseOK];
}

- (IBAction)cancel:(id)sender
{
    [[NSApplication sharedApplication] stopModalWithCode:NSModalResponseCancel];
}

- (void)controlValueDidChange:(FNDControlViewController *)control
{
    if (self.changeHandler)
    {
        self.changeHandler(self, control);
    }
}

@end

static int makeTag(const std::array<char, 4> &type)
{
    return (type[0]) | (type[1] << 8) | (type[2] << 16) | (type[3] << 24);
}

static int makeTag(int type)
{
    return type;
}

static std::array<char, 4> fromTag(int tag)
{
    return {
        static_cast<char>(tag & 255),
        static_cast<char>((tag >> 8) & 255),
        static_cast<char>((tag >> 16) & 255),
        static_cast<char>((tag >> 24) & 255)
    };
}

class UIValueItem {
public:
    /*
     Create a menu from a title, tag container of either
     - std::pair<std::string, int>
     or
     - std::pair<std::string, std::array<char, 4>>
     */
    template <class T>
    UIValueItem(int identifier, const std::string &title, const T &items, int selectedTag)
    : controller_([[FNDControlViewController alloc] init])
    {
        NSPopUpButton *menu = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 80, 25) pullsDown:NO];
        for (const auto &next : items)
        {
            [menu addItemWithTitle:[NSString stringWithCString:next.second.c_str() encoding:NSUTF8StringEncoding]];
            menu.lastItem.tag = makeTag(next.first);
        }
        [menu sizeToFit];

        NSTextField *label = makeLabel(title);

        controller_.controlIdentifier = identifier;
        controller_.selectedTag = selectedTag;
        controller_.useAuto = NO;
        [menu bind:@"selectedTag" toObject:controller_ withKeyPath:@"selectedTag" options:nil];
        [menu bind:@"enabled" toObject:controller_ withKeyPath:@"enabled" options:nil];
        controller_.view = [NSStackView stackViewWithViews:@[label, menu]];
    };
    /*
     Create a stepper - for now only integer values
     */
    UIValueItem(int identifier, const std::string &title, int  minimum, int maximum, int current, bool hasAuto, bool currentAuto)
    : controller_([[FNDControlViewController alloc] init])
    {
        controller_.controlIdentifier = identifier;
        controller_.doubleValue = current;
        controller_.useAuto = currentAuto;

        NSStepper *stepper = [[NSStepper alloc] init];
        stepper.maxValue = maximum;
        stepper.minValue = minimum;
        [stepper bind:@"value" toObject:controller_ withKeyPath:@"doubleValue" options:nil];

        NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
        formatter.numberStyle = NSNumberFormatterNoStyle;
        formatter.minimum = @(minimum);
        formatter.maximum = @(maximum);

        NSTextField *field = [NSTextField textFieldWithString:[formatter stringFromNumber:@(maximum)]];

        field.formatter = formatter;

        [field sizeToFit];

        [field bind:@"value" toObject:controller_ withKeyPath:@"doubleValue" options:nil];

        field.stringValue = [formatter stringFromNumber:@(current)];

        NSTextField *label = makeLabel(title);

        if (hasAuto)
        {
            NSButton *check = [NSButton checkboxWithTitle:@"Auto" target:nil action:nil];
            [check bind:@"value" toObject:controller_ withKeyPath:@"useAuto" options:nil];

            NSDictionary *option = @{NSValueTransformerBindingOption:
                                         [NSValueTransformer valueTransformerForName:NSNegateBooleanTransformerName]};

            [field bind:@"enabled" toObject:controller_ withKeyPath:@"useAuto" options:option];
            [stepper bind:@"enabled" toObject:controller_ withKeyPath:@"useAuto" options:option];

            controller_.view = [NSStackView stackViewWithViews:@[label, check, field, stepper]];
        }
        else
        {
            controller_.view = [NSStackView stackViewWithViews:@[label, field, stepper]];
        }
    };
    int getIdentifier() const { return controller_.controlIdentifier; };
    NSView *getContainerView() const { return controller_.view; };
    int getSelectedMenuItemTag() const
    {
        return controller_.selectedTag;
    }
    int getStepperValue() const
    {
        return controller_.doubleValue;
    }
    bool getEnabled() const
    {
        return controller_.enabled;
    }
    void setEnabled(bool enabled)
    {
        controller_.enabled = enabled;
    }
    void setDelegate(id <FNDControlDelegate> delegate)
    {
        controller_.delegate = delegate;
    }
    bool getUseAuto() const
    {
        return controller_.useAuto;
    }
private:
    static NSTextField *makeLabel(const std::string &title)
    {
        NSString *titleString = [NSString stringWithUTF8String:title.c_str()];
        if (![titleString hasSuffix:@":"])
        {
            titleString = [titleString stringByAppendingString:@":"];
        }
        return [NSTextField labelWithString:titleString];
    }

    FNDControlViewController *controller_{nil};
};

enum class UIItem : int {
    SubType,
    Quality,
    Chunks
};

class UIView {
public:
    UIView()
    : controller_([[FNDAEXViewController alloc] initWithNibName:@"SettingsViewController"
                                                         bundle:[NSBundle bundleWithIdentifier:FOUNDATION_MACOSX_BUNDLE_GUI_IDENTIFIER]])
    {
        controller_.changeHandler = ^(FNDAEXViewController *view, FNDControlViewController *control) {
            this->validateChange(control);
        };
        // Cause the view to actually be loaded so we have our outlets available
        [controller_ view];
    };
    void addItem(UIValueItem &&item)
    {
        item.setDelegate((id <FNDControlDelegate>)controller_);
        [controller_.stackView addView:item.getContainerView() inGravity:NSStackViewGravityLeading];
        items_.push_back(std::move(item));
    }
    UIValueItem *getItem(int identifier)
    {
        for (auto &next : items_)
        {
            if (next.getIdentifier() == identifier)
            {
                return &next;
            }
        }
        return nullptr;
    }
    NSView *getView() const
    {
        return controller_.view;
    }
    void validateChange(FNDControlViewController *control)
    {
        // TODO: this should be reaching outside the foundation
        // for codec-speficic validation of controls in platform-agnostic code

        if (static_cast<UIItem>(control.controlIdentifier) == UIItem::SubType)
        {
            const auto &codec = CodecRegistry::codec();
            bool enable = codec->hasQuality(fromTag(control.selectedTag));

            getItem(static_cast<int>(UIItem::Quality))->setEnabled(enable);
        }
    }
private:
    FNDAEXViewController *controller_;
    std::vector<UIValueItem> items_;
};

bool ui_OutDialog(CodecSubType& subType, int &quality, int& chunkCount, void *platformSpecific)
{
    bool didSave = false;
    const auto& codec = *CodecRegistry::codec();

    @autoreleasepool {
        UIView view;

        const auto &subtypes = codec.details().subtypes;
        if (!subtypes.empty())
        {
            // TODO: user-facing terminology, for all uses
            // NSView *menuView = buildMenu("Sub-Type", subtypes, subType);
            view.addItem(UIValueItem(static_cast<int>(UIItem::SubType), "Sub-Type", subtypes, makeTag(subType)));
        }

        if (codec.hasQualityForAnySubType())
        {
            view.addItem(UIValueItem(static_cast<int>(UIItem::Quality), "Quality", codec.qualityDescriptions(), quality));
        }

        if (codec.details().hasChunkCount)
        {
            // Alternatively auto could be split out into its own parameter
            view.addItem(UIValueItem(static_cast<int>(UIItem::Chunks), "Chunks", 1, 8, chunkCount == 0 ? 1 : chunkCount, true, chunkCount == 0));
        }

        NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0.0, 0.0, 100.0, 100.0)
                                                       styleMask:NSWindowStyleMaskTitled
                                                         backing:NSBackingStoreBuffered
                                                           defer:YES];
        // ARC releases the window
        window.releasedWhenClosed = NO;

        window.title = [NSString stringWithUTF8String:codec.details().productName.c_str()];

        window.contentView = view.getView();

        [window setContentSize:window.contentView.fittingSize];
        [window center];

        NSModalResponse response = [[NSApplication sharedApplication] runModalForWindow:window];
        if (response == NSModalResponseOK)
        {
            subType = fromTag(view.getItem(static_cast<int>(UIItem::SubType))->getSelectedMenuItemTag());
            quality = view.getItem(static_cast<int>(UIItem::Quality))->getSelectedMenuItemTag();
            UIValueItem *chunks = view.getItem(static_cast<int>(UIItem::Chunks));
            chunkCount = chunks->getUseAuto() ? 0 : chunks->getStepperValue();
            didSave = true;
        }
        [window close];
    }
	
    return didSave;
}
