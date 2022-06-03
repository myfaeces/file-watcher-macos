
/**
 * - Filesystem events are recursive. If you want then not to be recursive, implement it yourself (by dropping events from subdirectories).
 * — You can ask api to watch multiple paths at once.
 * - After you create the thing, CoreServices's daemon will call the callback (that you provided) on a separate thread, created and managed by the operating system.
 * — Each event you receive will have three fields: full path, os internal timestamp-like id (called eventId) and flags (or'ed together). (With some work you could alse receive the inode number).
 * — There is a mechanism to collect changes from a specific time. Which is cool. There are two ways to do that: you either remember the last eventId (that was mentioned ealier), or use a function that converts absolute time to eventId.
 * — Somethimes seemingly one operation will generate two events if source and destination are both watched. For example: 'mv oldname newname'.
 * — You create watcher once with a set list of paths. If you want to modify said list, tough luck, you cannot.
 *
 *
 * To build run:
 *
 *   /path/to/clang -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -framework CoreServices -std=c++11 -o fs_watcher -x c++ fs_watcher.cpp
 *
 * where '/path/to/clang/copiler' is a path to clang,
 *   and '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk' is a path to macos SDK, which you can check using the 'xcrun --show-sdk-path' command.
 *
 */

#include <CoreServices/CoreServices.h>

struct Watcher_Context_Example {
    // These are required, but you'd add your userdata in this struct.
    FSEventStreamRef     fs_event_stream;
    dispatch_queue_t     dispatch_queue;
    FSEventStreamContext fs_event_stream_context;
};

namespace Event_Type_Example {
enum Enum {
    OTHER,
    CREATED,
    MODIFIED,
    RENAMED,
    REMOVED,
    ATTR_MODIFIED,
    SOME_CHANGE_SOMEWHERE_PLEASE_RESCAN,
};
};

void watcher_start(Watcher_Context_Example *watcher_context, const char **paths, int paths_count);
void watcher_stop(Watcher_Context_Example *watcher_context);
void watcher_callback_example(Watcher_Context_Example *watcher_context, const char *path, Event_Type_Example::Enum event_type);

int main() {
    // This is an example list of directories. We do not chech if these paths are actually exist.
    // Note, that if you pass directories such that one is a subpath of another (for example "./dir" and "./dir/subdir"), you will receive only one message per event.
    //
    //                      ***    MODIFY THESE   ***
    const char *paths[] = { "./dir1", "./dir2/sub1" };
    int paths_count     = sizeof(paths) / sizeof(paths[0]);

    // This data passed between procedures, add your userdata anywhere in that struct.
    Watcher_Context_Example watcher_context = {};

    // I.   Start file watcher on the list of paths. It will watch them recursively.
    watcher_start(&watcher_context, paths, paths_count);

    // II.  Block this thread so we don't melt the cpu (or do something else). Another thread will be created and managed by the os. Let's not mention one million other threads with pegging your cpu, spawnded by this CoreFramework to feed the Spotlight.
    printf("\nWatching paths. Press ENTER to stop ...\n\n");
    while('\n' != getchar());

    // III. Sink all remaining events and stop watcher.
    watcher_stop(&watcher_context);
}





void watcher_callback_example(Watcher_Context_Example *watcher_context, const char *path, Event_Type_Example::Enum event_type) {
    switch (event_type)
    {
        // If you copy or move some folder with some stuff in it, 'CREATED' will trigger single time for that folder (and will not trigger for its children).
        case Event_Type_Example::CREATED:       printf("%s - %s\n", "[ created ]", path); break;

        // 'REMOVED' will trigger for every file and folder recursively.
        // Note, that 'REMOVED' will not trigger if you put items into the recycle bin (or whatever its called), instead 'RENAMED' will occur.
        case Event_Type_Example::REMOVED:       printf("%s - %s\n", "[ removed ]", path); break;

        // By 'RENAMED' we mean that something in /full/path/to/file was modified.
        // Examples:
        //     mv /some/unwatched/dir/src_file /some/watched/dir/dst_file    // one event : dst_file
        //     mv /some/watched/dir/src_file   /some/unwatched/dir/dst_file  // one event : src_file
        //     mv /some/unwatched/dir/src_file /some/watched/dir/dst_file    // two events: src_file AND dst_file
        case Event_Type_Example::RENAMED:       printf("%s - %s\n", "[ renamed ]", path); break;

        case Event_Type_Example::MODIFIED:      printf("%s - %s\n", "[ modified]", path); break;
        case Event_Type_Example::ATTR_MODIFIED: printf("%s - %s\n", "[attribute]", path); break;

        case Event_Type_Example::SOME_CHANGE_SOMEWHERE_PLEASE_RESCAN: printf("%s - %s\n", "[   ???   ]", path); break;

        case Event_Type_Example::OTHER:         printf("%s - %s\n", "[  other  ]", path); break;
    }
}

static void fs_event_stream_callback_example(ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents, void *eventPaths, const FSEventStreamEventFlags *eventFlags, const FSEventStreamEventId *eventIds) {
    auto watcher_context = (Watcher_Context_Example*)clientCallBackInfo;
    auto event_paths     = (const char **)eventPaths;

    for (size_t i = 0; i < numEvents; ++i) {
        const char *path              = event_paths[i];
        FSEventStreamEventFlags flag  = eventFlags[i];
        FSEventStreamEventId event_id = eventIds[i];

        Event_Type_Example::Enum event_type;

        // @refer to the FSEvents.h header for flags explanation.
        //
#if 0
        printf("%s:\n", path);
        if (flag == kFSEventStreamEventFlagNone)              printf("\t%s\n", "kFSEventStreamEventFlagNone");
        if (flag & kFSEventStreamEventFlagMustScanSubDirs)    printf("\t%s\n", "kFSEventStreamEventFlagMustScanSubDirs");
        if (flag & kFSEventStreamEventFlagUserDropped)        printf("\t%s\n", "kFSEventStreamEventFlagUserDropped");
        if (flag & kFSEventStreamEventFlagKernelDropped)      printf("\t%s\n", "kFSEventStreamEventFlagKernelDropped");
        if (flag & kFSEventStreamEventFlagEventIdsWrapped)    printf("\t%s\n", "kFSEventStreamEventFlagEventIdsWrapped");
        if (flag & kFSEventStreamEventFlagHistoryDone)        printf("\t%s\n", "kFSEventStreamEventFlagHistoryDone");
        if (flag & kFSEventStreamEventFlagRootChanged)        printf("\t%s\n", "kFSEventStreamEventFlagRootChanged");
        if (flag & kFSEventStreamEventFlagMount)              printf("\t%s\n", "kFSEventStreamEventFlagMount");
        if (flag & kFSEventStreamEventFlagUnmount)            printf("\t%s\n", "kFSEventStreamEventFlagUnmount");
        if (flag & kFSEventStreamEventFlagItemCreated)        printf("\t%s\n", "kFSEventStreamEventFlagItemCreated");
        if (flag & kFSEventStreamEventFlagItemRemoved)        printf("\t%s\n", "kFSEventStreamEventFlagItemRemoved");
        if (flag & kFSEventStreamEventFlagItemInodeMetaMod)   printf("\t%s\n", "kFSEventStreamEventFlagItemInodeMetaMod");
        if (flag & kFSEventStreamEventFlagItemRenamed)        printf("\t%s\n", "kFSEventStreamEventFlagItemRenamed");
        if (flag & kFSEventStreamEventFlagItemModified)       printf("\t%s\n", "kFSEventStreamEventFlagItemModified");
        if (flag & kFSEventStreamEventFlagItemFinderInfoMod)  printf("\t%s\n", "kFSEventStreamEventFlagItemFinderInfoMod");
        if (flag & kFSEventStreamEventFlagItemChangeOwner)    printf("\t%s\n", "kFSEventStreamEventFlagItemChangeOwner");
        if (flag & kFSEventStreamEventFlagItemXattrMod)       printf("\t%s\n", "kFSEventStreamEventFlagItemXattrMod");
        if (flag & kFSEventStreamEventFlagItemIsFile)         printf("\t%s\n", "kFSEventStreamEventFlagItemIsFile");
        if (flag & kFSEventStreamEventFlagItemIsDir)          printf("\t%s\n", "kFSEventStreamEventFlagItemIsDir");
        if (flag & kFSEventStreamEventFlagItemIsSymlink)      printf("\t%s\n", "kFSEventStreamEventFlagItemIsSymlink");
        if (flag & kFSEventStreamEventFlagOwnEvent)           printf("\t%s\n", "kFSEventStreamEventFlagOwnEvent");
        if (flag & kFSEventStreamEventFlagItemIsHardlink)     printf("\t%s\n", "kFSEventStreamEventFlagItemIsHardlink");
        if (flag & kFSEventStreamEventFlagItemIsLastHardlink) printf("\t%s\n", "kFSEventStreamEventFlagItemIsLastHardlink");
        if (flag & kFSEventStreamEventFlagItemCloned)         printf("\t%s\n", "kFSEventStreamEventFlagItemCloned");
#else
        if (flag == kFSEventStreamEventFlagNone)
            // Received all the time if you do not set 'kFSEventStreamCreateFlagFileEvents' flag.
            event_type = Event_Type_Example::SOME_CHANGE_SOMEWHERE_PLEASE_RESCAN;
        else if (flag & kFSEventStreamEventFlagMustScanSubDirs)
            // Looks like this happens then CoreServices barfs.
            event_type = Event_Type_Example::SOME_CHANGE_SOMEWHERE_PLEASE_RESCAN;
        else if (flag & (kFSEventStreamEventFlagItemChangeOwner|kFSEventStreamEventFlagItemInodeMetaMod|kFSEventStreamEventFlagItemFinderInfoMod))
            // 'kFSEventStreamEventFlagItemChangeOwner' is before 'kFSEventStreamEventFlagItemCreated' because if you do for example 'chmod +x file' event will have both flags.
            event_type = Event_Type_Example::ATTR_MODIFIED;
        else if (flag & kFSEventStreamEventFlagItemCreated)
            event_type = Event_Type_Example::CREATED;
        else if (flag & kFSEventStreamEventFlagItemModified)
            event_type = Event_Type_Example::MODIFIED;
        else if (flag & kFSEventStreamEventFlagItemRenamed)
            event_type = Event_Type_Example::RENAMED;
        else if (flag & kFSEventStreamEventFlagItemRemoved)
            event_type = Event_Type_Example::REMOVED;
        else
            event_type = Event_Type_Example::OTHER;

        watcher_callback_example(watcher_context, path, event_type);
#endif
    }
}

static void dispatch_queue_stop(Watcher_Context_Example *watcher_context) {
    FSEventStreamStop(watcher_context->fs_event_stream);
    FSEventStreamInvalidate(watcher_context->fs_event_stream);
    FSEventStreamRelease(watcher_context->fs_event_stream);
}

void watcher_stop(Watcher_Context_Example *watcher_context) {
    dispatch_sync_f(watcher_context->dispatch_queue, watcher_context, (dispatch_function_t)dispatch_queue_stop);
    dispatch_release(watcher_context->dispatch_queue);
}

static void dispatch_queue_start(Watcher_Context_Example *watcher_context) {
    FSEventStreamSetDispatchQueue(watcher_context->fs_event_stream, watcher_context->dispatch_queue);
    FSEventStreamStart(watcher_context->fs_event_stream);
}

void watcher_start(Watcher_Context_Example *watcher_context, const char **paths, int paths_count) {
    // We use File System Events api which is a part of the CoreServices framework:
    // https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/FSEvents_ProgGuide/Introduction/Introduction.html
    // https://developer.apple.com/library/archive/documentation/CoreFoundation/Conceptual/CFDesignConcepts/CFDesignConcepts.html
    // https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/Architecture/Architecture.html
    // , but there is an alternative:
    // https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/FSEvents_ProgGuide/KernelQueues/KernelQueues.html

    // @trivia
    // 'CF' in apple naming stands for "Core Foundation".
    // Certain apple's typedefs are interchangeable between frameworks, for example NSString - CFStringRef, or NSArray - CFArrayRef, and others.
    // And of cource there is a name for that:
    // https://developer.apple.com/library/archive/documentation/General/Conceptual/CocoaEncyclopedia/Toll-FreeBridgin/Toll-FreeBridgin.html


    CFMutableArrayRef cf_paths = CFArrayCreateMutable(NULL, (CFIndex)paths_count, &kCFTypeArrayCallBacks);
    for (int i = 0; i < paths_count; ++i) {
        const char *path = paths[i];
        CFStringRef cf_path = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
        CFArrayAppendValue(cf_paths, cf_path);
        CFRelease(cf_path); // Because 'cf_path' was copied into 'cf_paths' array, we should release it.
    }



    //                                *** MODIFY THESE FLAGS ***
    //                         (comprehensive explanation in FSEvents.h)
    //
    // - sinceWhen                               // Some sort of monotonic clock that is persistentent across reboots.
    //       kFSEventStreamEventIdSinceNow       // Track changes from this moment.
    //       or pass a previous id               // You receive it in the `eventIds` argument. If you do that, then os will pack and send you all changes that happened from that time (hence the 'sinceWhen' name).
    // - latency                                 // Number of seconds os should wait before calling our callback (os will merge all events from this time into one).
    // - flags
    //       kFSEventStreamCreateFlagNone        // Default.
    //       kFSEventStreamCreateFlagUseCFTypes  // Recieve paths as CF types (instead of regular `char*`).
    //       kFSEventStreamCreateFlagNoDefer     // More real-time updates (read header for explanation).
    //       kFSEventStreamCreateFlagWatchRoot   // Receive events when one of the root directories changes.
    //       kFSEventStreamCreateFlagIgnoreSelf  // Don't send events that were triggered by the current process.
    //       kFSEventStreamCreateFlagFileEvents  // HIGHLY RECOMMENDED. Receive events about individual files. Otherwise you will receive events about directories only (including subdirectories).
    //                                           // Trade small cpu usage for much better information. Your cpu will be pegged by the Spotlight regardless of what you do here (watch your cpu during 'Empty Bin' operation).
    //                                           // For some reason if you do not set this flag, almost every event will be received without flags (or kFSEventStreamEventFlagNone, which is 0).
    //       kFSEventStreamCreateFlagMarkSelf    // Events that were triggered by the current process with the "OwnEvent" flag.
    //       kFSEventStreamCreateFlagUseExtendedData // Receive info as CFDictionaryRefs which has path and inode number.
    //       kFSEventStreamCreateFlagFullHistory // Something to do with persistent history (see `sinceWhen` parameter).
    CFAllocatorRef           allocator    = NULL;
    FSEventStreamCallback    callback     = (FSEventStreamCallback)fs_event_stream_callback_example;
    FSEventStreamContext    *context      = &watcher_context->fs_event_stream_context;
    context->info = watcher_context;
    CFArrayRef               pathsToWatch = cf_paths;
    FSEventStreamEventId     sinceWhen    = kFSEventStreamEventIdSinceNow;
    CFTimeInterval           latency      = 0.0;
    FSEventStreamCreateFlags flags        = kFSEventStreamCreateFlagFileEvents|kFSEventStreamCreateFlagNoDefer;

    watcher_context->fs_event_stream = FSEventStreamCreate(allocator, callback, context, pathsToWatch, sinceWhen, latency, flags);
    CFRelease(cf_paths); // 'cf_paths' array was copied (apple loves duplicate data, isn't it) into FSStream somewhere, so we should release it.
    assert(watcher_context->fs_event_stream);



    // Basically os's scheduler wants to know on which thread to call our callback. Or something like that.
    // But since apple doesn't like threads (read above), it wants either a RunLoop or a DispatchQueue (read above).
    // We pass it a queue, as passing runloop with 'FSEventStreamScheduleWithRunLoop' procedure is (or soon will be) deprecated.
    //
    // Note, that we start a FSEventStream from inside this queue to avoid races.
    //
    dispatch_queue_t dispatch_queue = dispatch_queue_create("MyFileWatcher", DISPATCH_QUEUE_SERIAL);
    watcher_context->dispatch_queue = dispatch_queue;
    dispatch_sync_f(dispatch_queue, watcher_context, (dispatch_function_t)dispatch_queue_start);
    //
    // @trivia
    // Quote from apple's online documentation:
    //     Although threads have been around for many years and continue to have their uses,
    //     they do not solve the general problem of executing multiple tasks in a scalable way.
    //     With threads, the burden of creating a scalable solution rests squarely on the shoulders of you, the developer.
    //
    // @read
    // https://developer.apple.com/library/archive/documentation/General/Conceptual/ConcurrencyProgrammingGuide/ConcurrencyandApplicationDesign/ConcurrencyandApplicationDesign.html
    // https://developer.apple.com/library/archive/documentation/General/Conceptual/ConcurrencyProgrammingGuide/ThreadMigration/ThreadMigration.html
    // https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Multithreading/RunLoopManagement/RunLoopManagement.html
    // https://developer.apple.com/documentation/dispatch
    // https://developer.apple.com/documentation/dispatch/dispatch_queue
}
