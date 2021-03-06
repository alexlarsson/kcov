#include <configuration.hh>
#include <reporter.hh>
#include <writer.hh>
#include <collector.hh>
#include <output-handler.hh>
#include <elf.hh>

#include <string.h>
#include <signal.h>

#include "html-writer.hh"

using namespace kcov;

static IOutputHandler *g_output;
static ICollector *g_collector;
static IReporter *g_reporter;

static void ctrlc(int sig)
{
	g_collector->stop();
	g_reporter->stop();
	g_output->stop();
	exit(0);
}


int main(int argc, const char *argv[])
{
	IConfiguration &conf = IConfiguration::getInstance();

	if (!conf.parse(argc, argv))
		return 1;

	conf.setOutputType(IConfiguration::OUTPUT_PROFILER);

	std::string file = conf.getBinaryPath() + conf.getBinaryName();
	IElf *elf = IElf::open(file.c_str());
	if (!elf) {
		conf.printUsage();
		return 1;
	}

	ICollector &collector = ICollector::create(elf);
	IReporter &reporter = IReporter::create(*elf, collector);
	IOutputHandler &output = IOutputHandler::create(reporter);

	// Register writers
	IWriter &htmlWriter = createHtmlWriter(*elf, reporter, output);
	output.registerWriter(htmlWriter);

	g_output = &output;
	g_reporter = &reporter;
	g_collector = &collector;
	signal(SIGINT, ctrlc);
	signal(SIGTERM, ctrlc);

	output.start();
	int ret = collector.run();
	output.stop();

	return ret;
}
