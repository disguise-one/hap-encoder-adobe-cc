#import "FNDControlViewController.h"

@implementation FNDControlViewController {
    NSInteger   _selectedTag;
    double      _doubleValue;
}

- (instancetype)initWithNibName:(NSNibName)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self)
    {
        _enabled = YES;
        _useAuto = NO;
    }
    return self;
}

-(NSInteger)selectedTag
{
    return _selectedTag;
}

- (void)setSelectedTag:(NSInteger)selectedTag
{
    _selectedTag = selectedTag;
    [self.delegate controlValueDidChange:self];
}

- (double)doubleValue
{
    return _doubleValue;
}

- (void)setDoubleValue:(double)doubleValue
{
    _doubleValue = doubleValue;
    [self.delegate controlValueDidChange:self];
}

@end
