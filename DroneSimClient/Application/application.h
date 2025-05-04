#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>

/// <summary>
/// Главный класс приложения
/// </summary>
class Application : public QApplication
{
	Q_OBJECT

public:
    Application(int &argc, char **argv);
    virtual ~Application();

	/// <summary>
    /// Запуск
	/// </summary>
	/// <returns>Возвращает код цикла обработки событий</returns>
	int run();
};


#endif // APPLICATION_H
