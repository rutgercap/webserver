#include <CGI.hpp>
#include <unistd.h>

/**************************************************/
/* Constructors / destructors		 	          */
/**************************************************/
// TODO: implement getenv
CGI::CGI(std::string const &rootDir, char *const *env) : _rootDir(rootDir), _env(env) {}

CGI::~CGI() {}

/**************************************************/
/* CGI methods						 	          */
/**************************************************/

/* Child process that executes CGI file */
int CGI::forkCgiFile(int fd[2], std::string const &filePath, std::string const &body) const
{
	Logger &logger = Logger::getInstance();
	std::string fullPath = _rootDir + filePath;

	char *argv[] = {
		const_cast<char *>(PATH_TO_PYTHON),
		const_cast<char *>(fullPath.c_str()),
		const_cast<char *>(body.c_str())};
	close(fd[READ]);
	if (dup2(fd[WRITE], STDOUT_FILENO) == -1)
	{
		logger.error("[EXECUTING] CGI: dup2: " + std::string(strerror(errno)));
		close(fd[WRITE]);
		exit(1);
	}
	if (dup2(fd[WRITE], STDERR_FILENO) == -1)
	{
		logger.error("[EXECUTING] CGI: dup2: " + std::string(strerror(errno)));
		close(fd[WRITE]);
		exit(1);
	}
	if (execve(PATH_TO_PYTHON, static_cast<char *const *>(argv), _env))
	{
		logger.error("[EXECUTING] CGI: execve: " + std::string(strerror(errno)));
		exit(1);
	}
	return 0;
}

static int waitForChildProcess(pid_t const &pid)
{
	int status;

	while (waitpid(pid, &status, 0) != -1)
	{
	}

	if (WIFEXITED(status))
	{
		// TODO: Replace to_string with c98 function??
		Logger::getInstance().error("[EXECUTING] CGI: child process exited with: " + std::to_string(WEXITSTATUS(status)));
		return WEXITSTATUS(status);
	}
	return 0;
}

/* Waits for input from child process and puts it in pDest */
static int readFromChildProcess(std::string *pDest, pid_t const &pid, int fd[2])
{
	if (waitForChildProcess(pid) != 0)
	{
		return 1;
	}

	std::string output("");
	std::string buffer[BUFFER_SIZE + 1];
	int bytesRead;
	do
	{
		bytesRead = read(fd[READ], &buffer, BUFFER_SIZE);
		if (bytesRead == -1)
		{
			Logger::getInstance().error("[EXECUTING] CGI: read: " + std::string(strerror(errno)));
			return 1;
		}
		output += bytesRead;
	} while (bytesRead > 0);

	*pDest = output;
	close(fd[WRITE]);
	close(fd[READ]);
	return 0;
}

/* CGI control flow */
int CGI::executeFile(std::string *pDest, std::string const &filePath, std::string const &body) const
{
	Logger &logger = Logger::getInstance();
	int fd[2];
	pid_t pid;

	Logger::getInstance().log("[STARTING] CGI ");

	/* Open pipe */
	if (pipe(fd) == -1)
	{
		logger.error("[PREPARING] CGI: pipe: " + std::string(strerror(errno)));
		throw std::runtime_error("[PREPARING] CGI: pipe: " + std::string(strerror(errno)));
	}

	/* Start fork */
	pid = fork();
	if (pid == -1)
	{
		logger.error("[PREPARING] CGI: fork: " + std::string(strerror(errno)));
		throw std::runtime_error("[PREPARING] CGI: fork: " + std::string(strerror(errno)));
	}
	else if (pid == CHILD)
	{
		if (forkCgiFile(fd, filePath, body))
		{
			throw std::runtime_error("[EXECUTING] CGI: " + std::string(strerror(errno)));
		}
	}
	else
	{
		if (readFromChildProcess(pDest, pid, fd))
		{
			throw std::runtime_error("[EXECUTING] CGI: " + std::string(strerror(errno)));
		}
	}
	return 0;
}