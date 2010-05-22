//
//  SUUnarchiver.m
//  Sparkle
//
//  Created by Andy Matuschak on 3/16/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//


#import "Sparkle.h"
#import "SUUnarchiver.h"
#import "SUUnarchiver_Private.h"

@implementation SUUnarchiver

+ (SUUnarchiver *)unarchiverForPath:(NSString *)path
{
	NSEnumerator *implementationEnumerator = [[self unarchiverImplementations] objectEnumerator];
	id current;
	while ((current = [implementationEnumerator nextObject]))
	{
		if ([current canUnarchivePath:path])
			return [[[current alloc] initWithPath:path] autorelease];
	}
	return nil;
}

- (NSString *)description { return [NSString stringWithFormat:@"%@ <%@>", [self class], archivePath]; }

- (void)setDelegate:del
{
	delegate = del;
}

- (void)start
{
	// No-op
}

@end
