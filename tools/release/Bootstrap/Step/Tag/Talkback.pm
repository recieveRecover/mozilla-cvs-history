#
# Tag step. Applies a CVS tag to the appropriate repositories.
# 
package Bootstrap::Step::Tag::Talkback;

use strict;

use File::Copy qw(move);

use MozBuild::Util qw(MkdirWithPath);

use Bootstrap::Util qw(CvsCatfile);
use Bootstrap::Step;
use Bootstrap::Config;
use Bootstrap::Step::Tag;

our @ISA = ("Bootstrap::Step::Tag");

sub Execute {
    my $this = shift;

    my $config = new Bootstrap::Config();
    my $productTag = $config->Get(var => 'productTag');
    my $branchTag = $config->Get(var => 'branchTag');
    my $build = int($config->Get(var => 'build'));
    my $pullDate = $config->Get(var => 'pullDate');
    my $logDir = $config->Get(sysvar => 'logDir');
    my $mofoCvsroot = $config->Get(var => 'mofoCvsroot');
    my $tagDir = $config->Get(var => 'tagDir');

    my $releaseTag = $productTag . '_RELEASE';
    my $buildTag = $productTag . '_BUILD' . $build;
    my $releaseTagDir = catfile($tagDir, $buildTag);

    # Since talkback so seldom changes, we don't include it in our fancy
    # respin logic; we only need to tag it for build 1.
    if ($build > 1) {
        $this->Log(msg => "Not tagging Talkback repo for build $build.");
        return;
    }

    # Create the mofo tag directory.
    my $mofoDir = catfile($releaseTagDir, 'mofo');
    if (not -d $mofoDir) {
        MkdirWithPath(dir => $mofoDir) 
          or die("Cannot mkdir $mofoDir: $!");
    }

    # Check out the talkback files from the branch you want to tag.
    $this->CvsCo(
      cvsroot => $mofoCvsroot,
      tag => $branchTag,
      date => $pullDate,
      modules => [CvsCatfile('talkback', 'fullsoft')],
      workDir => catfile($releaseTagDir, 'mofo'),
      logFile => catfile($logDir, 'tag-talkback_mofo-checkout.log')
    );

    # Create the talkback RELEASE tag.
    $this->CvsTag(
      tagName => $releaseTag,
      coDir => catfile($releaseTagDir, 'mofo', 'talkback', 'fullsoft'),
      logFile => catfile($logDir, 
                         'tag-talkback_mofo-tag-' . $releaseTag . '.log'),
    );
}

sub Verify {
    my $this = shift;

    my $config = new Bootstrap::Config();
    my $logDir = $config->Get(sysvar => 'logDir');
    my $productTag = $config->Get(var => 'productTag');
    my $build = $config->Get(var => 'build');

    if ($build > 1) {
        $this->Log(msg => "Not verifying Talkback repo for build $build.");
        return;
    }

    my $releaseTag = $productTag . '_RELEASE';

    $this->CheckLog(
      log => catfile($logDir, 
                     'tag-talkback_mofo-tag-' . $releaseTag . '.log'),
      checkFor => '^T',
    );
}

1;
