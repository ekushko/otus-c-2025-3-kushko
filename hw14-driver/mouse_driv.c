/*
 * mouse_driv.c - Простой драйвер мыши c эмуляцией считывания координат
 * Разрабатывалось на Linux 6.11.0-19-generic
 * Этот модуль ядра создает символьное устройство для отслеживания движений мыши,
 * позволяет выполнять операции чтения и записи, и предоставляет настройку через sysfs.
 *
 * === ИНСТРУКЦИИ ПО ИСПОЛЬЗОВАНИЮ ===
 *
 * УСТАНОВКА:
 * 0. make
 * 1. Вставьте модуль: sudo insmod mouse_driv.ko
 * 2. Проверьте dmesg, чтобы убедиться, что модуль загрузился корректно: sudo dmesg | tail
 * 3. Создайте узел устройства вручную: sudo mknod /dev/mouselog c <major_number> 0 (узнайте major_number из вывода dmesg)
 * 4. Установите права доступа: sudo chmod <major_number> /dev/mouselog
 *
 * ИСПОЛЬЗОВАНИЕ:
 * 1. Чтение данных о движении мыши: cat /dev/mouselog
 * 2. Запись в устройство: echo "какой-то текст" > /dev/mouselog
 *
 * НАСТРОЙКА ЧЕРЕЗ SYSFS:
 * Настройка режима отображения (0=только X, 1=только Y, 2=X и Y):
 *    - Чтение: cat /sys/mouselog/display_mode
 *    - Запись: echo 2 > /sys/mouselog/display_mode
 *
 * УДАЛЕНИЕ:
 * 1. Удалите модуль: sudo rmmod mouse_driv
 * 2. Удалите узел устройства: sudo rm /dev/mouselog
 * 3. Удалить файлы после сборки: make clean
 *
 * ТЕСТИРОВАНИЕ:
 * 1. Прочитайте файл устройства для получения данных о движении мыши:
 *    cat /dev/mouselog
 * 2. Запишите в устройство:
 *    echo "тест" > /dev/mouselog
 * 3. Измените режим отображения:
 *    echo 0 > /sys/mouselog/display_mode   # Только X
 *    echo 1 > /sys/mouselog/display_mode   # Только Y
 *    echo 2 > /sys/mouselog/display_mode   # X и Y
 */

#include <linux/init.h>       // Для __init и __exit
#include <linux/module.h>     // Для MODULE_*
#include <linux/kernel.h>     // Для printk
#include <linux/fs.h>         // Для register_chrdev и т.д.
#include <linux/uaccess.h>    // Для copy_to/from_user
#include <linux/slab.h>       // Для kmalloc
#include <linux/mutex.h>      // Для mutex
#include <linux/kobject.h>    // Для struct kobject
#include <linux/random.h>     // Для get_random_long

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux User");
MODULE_DESCRIPTION("Простой драйвер мыши");
MODULE_VERSION("0.1");

// Имя устройства
#define DEVICE_NAME "mouselog"

// Параметры модуля
static int buffer_size = 4096;        // Размер буфера по умолчанию
module_param(buffer_size, int, 0644);
MODULE_PARM_DESC(buffer_size, "Размер буфера для данных мыши");

// Режим отображения: 0 - только X, 1 - только Y, 2 - X и Y
static int display_mode = 2;
module_param(display_mode, int, 0644);
MODULE_PARM_DESC(display_mode, "Режим отображения (0=только X, 1=только Y, 2=X и Y)");

// Переменные для работы с символьным устройством
static int major_number;          // Старший номер устройства
static struct ring_buffer {
    char *data;
    size_t size;
    size_t head;
    size_t tail;
    struct mutex lock;
} buffer; // Буфер для хранения данных

// Структура для работы с sysfs
static struct kobject *mouse_kobj;

// Объявления функций
static int mouse_open(struct inode *, struct file *);
static int mouse_release(struct inode *, struct file *);
static ssize_t mouse_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t mouse_write(struct file *, const char __user *, size_t, loff_t *);

// Имитация данных мыши (для упрощения)
static void simulate_mouse_data(void);

// Операции с файлами устройства
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = mouse_open,
    .release = mouse_release,
    .read = mouse_read,
    .write = mouse_write,
};

// Функции для работы с атрибутами sysfs
static ssize_t display_mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", display_mode);
}

static ssize_t display_mode_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int value;

    // Преобразуем строку в число
    if (kstrtoint(buf, 10, &value) || value < 0 || value > 2)
        return -EINVAL;

    display_mode = value;
    return count;
}

static ssize_t buffer_status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    size_t available;
    mutex_lock(&buffer.lock);
    available = (buffer.head >= buffer.tail) ?
                (buffer.head - buffer.tail) :
                (buffer.size - buffer.tail + buffer.head);
    mutex_unlock(&buffer.lock);
    return sprintf(buf, "Used: %zu, Free: %zu\n", available, buffer.size - available - 1);
}

static ssize_t buffer_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%zu\n", buffer.size);
}

static ssize_t buffer_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    size_t new_size;
    char *new_data;

    if (kstrtoul(buf, 10, &new_size) < 0 || new_size < 64 || new_size > 65536)
        return -EINVAL;

    new_data = kmalloc(new_size, GFP_KERNEL);
    if (!new_data)
        return -ENOMEM;

    mutex_lock(&buffer.lock);
    if (buffer.head != buffer.tail) {
        kfree(new_data);
        mutex_unlock(&buffer.lock);
        return -EBUSY;
    }

    kfree(buffer.data);
    buffer.data = new_data;
    buffer.size = new_size;
    buffer.head = 0;
    buffer.tail = 0;
    mutex_unlock(&buffer.lock);
    return count;
}

static ssize_t clear_buffer_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int value;
    if (kstrtoint(buf, 10, &value) || value != 1)
        return -EINVAL;
    mutex_lock(&buffer.lock);
    buffer.head = 0;
    buffer.tail = 0;
    mutex_unlock(&buffer.lock);
    return count;
}

// Определение атрибутов sysfs
static struct kobj_attribute display_mode_attr = __ATTR(display_mode, 0644, display_mode_show, display_mode_store);
static struct kobj_attribute buffer_status_attr = __ATTR_RO(buffer_status);
static struct kobj_attribute buffer_size_attr = __ATTR(buffer_size, 0644, buffer_size_show, buffer_size_store);
static struct kobj_attribute clear_buffer_attr = __ATTR_WO(clear_buffer);

// Атрибуты, которые будут созданы в sysfs
static struct attribute *mouse_attrs[] = {
    &display_mode_attr.attr,
    &buffer_status_attr.attr,
    &buffer_size_attr.attr,
    &clear_buffer_attr.attr,
    NULL,
};

// Группа атрибутов (будет показана в sysfs)
static struct attribute_group attr_group = {
    .attrs = mouse_attrs,
};

// Функция инициализации буфера
static int ring_buffer_init(size_t size)
{
    buffer.data = kmalloc(size, GFP_KERNEL);
    if (!buffer.data) {
        printk(KERN_ERR "mouse_driv: Failed to allocate buffer\n");
        return -ENOMEM;
    }
    buffer.size = size;
    buffer.head = 0;
    buffer.tail = 0;
    mutex_init(&buffer.lock);
    return 0;
}

// Функция деинициализации буфера
static void ring_buffer_free(void)
{
    if (buffer.data) {
        kfree(buffer.data);
        buffer.data = NULL;
    }
}

// Функция записи в буфер
static ssize_t ring_buffer_write(const char __user *user_data, size_t count)
{
    size_t space, to_write;
    mutex_lock(&buffer.lock);

    space = (buffer.head >= buffer.tail) ?
            (buffer.size - (buffer.head - buffer.tail)) :
            (buffer.tail - buffer.head - 1);
    to_write = min(count, space);

    if (to_write == 0) {
        mutex_unlock(&buffer.lock);
        return -ENOSPC;
    }

    if (buffer.head + to_write <= buffer.size) {
        if (copy_from_user(buffer.data + buffer.head, user_data, to_write)) {
            mutex_unlock(&buffer.lock);
            return -EFAULT;
        }
        buffer.head += to_write;
    } else {
        size_t first_part = buffer.size - buffer.head;
        if (copy_from_user(buffer.data + buffer.head, user_data, first_part)) {
            mutex_unlock(&buffer.lock);
            return -EFAULT;
        }
        if (copy_from_user(buffer.data, user_data + first_part, to_write - first_part)) {
            mutex_unlock(&buffer.lock);
            return -EFAULT;
        }
        buffer.head = to_write - first_part;
    }

    mutex_unlock(&buffer.lock);
    return to_write;
}

// Функция чтения из буфера
static ssize_t ring_buffer_read(char __user *user_data, size_t count, loff_t *offset)
{
    size_t available, to_read;
    mutex_lock(&buffer.lock);

    available = (buffer.head >= buffer.tail) ?
                (buffer.head - buffer.tail) :
                (buffer.size - buffer.tail + buffer.head);
    to_read = min(count, available);

    if (to_read == 0) {
        mutex_unlock(&buffer.lock);
        return 0;
    }

    if (buffer.tail + to_read <= buffer.size) {
        if (copy_to_user(user_data, buffer.data + buffer.tail, to_read)) {
            mutex_unlock(&buffer.lock);
            return -EFAULT;
        }
        buffer.tail += to_read;
    } else {
        size_t first_part = buffer.size - buffer.tail;
        if (copy_to_user(user_data, buffer.data + buffer.tail, first_part)) {
            mutex_unlock(&buffer.lock);
            return -EFAULT;
        }
        if (copy_to_user(user_data + first_part, buffer.data, to_read - first_part)) {
            mutex_unlock(&buffer.lock);
            return -EFAULT;
        }
        buffer.tail = to_read - first_part;
    }

    mutex_unlock(&buffer.lock);
    *offset += to_read;
    return to_read;
}


// Функция для имитации данных мыши (простое решение)
static void simulate_mouse_data(void)
{
    char event_str[100];
    int event_len = 0;
    int x_value = get_random_long() % 10 - 5;  // Случайное число от -5 до 5
    int y_value = get_random_long() % 10 - 5;  // Случайное число от -5 до 5

    // Форматируем данные о событии в зависимости от режима отображения
    switch (display_mode) {
        case 0:  // Только X
            event_len = snprintf(event_str, sizeof(event_str), "MOUSE_X:%d\n", x_value);
            break;
        case 1:  // Только Y
            event_len = snprintf(event_str, sizeof(event_str), "MOUSE_Y:%d\n", y_value);
            break;
        case 2:  // X и Y
        default:
            event_len = snprintf(event_str, sizeof(event_str), "MOUSE_X:%d\nMOUSE_Y:%d\n", x_value, y_value);
            break;
    }

    // Заполняем буфер
    mutex_lock(&buffer.lock);

    size_t space = (buffer.head >= buffer.tail) ?
                   (buffer.size - (buffer.head - buffer.tail)) :
                   (buffer.tail - buffer.head - 1);
    if (event_len <= space) {
        if (buffer.head + event_len <= buffer.size) {
            memcpy(buffer.data + buffer.head, event_str, event_len);
            buffer.head += event_len;
        } else {
            size_t first_part = buffer.size - buffer.head;
            memcpy(buffer.data + buffer.head, event_str, first_part);
            memcpy(buffer.data, event_str + first_part, event_len - first_part);
            buffer.head = event_len - first_part;
        }
    }
    mutex_unlock(&buffer.lock);
}

// Обработчик открытия файла устройства
static int mouse_open(struct inode *inode, struct file *file)
{
    // Генерируем новые данные при каждом открытии
    simulate_mouse_data();
    return 0;
}

// Обработчик закрытия файла устройства
static int mouse_release(struct inode *inode, struct file *file)
{
    return 0;
}

// Обработчик чтения из файла устройства
static ssize_t mouse_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    return ring_buffer_read(buf, len, offset);
}

// Обработчик записи в файл устройства
static ssize_t mouse_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    return ring_buffer_write(buf, len);
}

// Функция инициализации модуля
static int __init mouse_driv_init(void)
{
    // Выделяем и инициализируем буфер
    int ret;
    if ((ret = ring_buffer_init(buffer_size)) < 0)
        return ret;

    // Регистрируем символьное устройство
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "Не удалось зарегистрировать старший номер\n");
        ring_buffer_free();
        return major_number;
    }

    // Создаем sysfs объект
    mouse_kobj = kobject_create_and_add("mouselog", NULL);
    if (!mouse_kobj) {
        printk(KERN_ERR "Не удалось создать sysfs объект\n");
        unregister_chrdev(major_number, DEVICE_NAME);
        ring_buffer_free();
        return -ENOMEM;
    }

    // Создаем sysfs атрибуты
    if (sysfs_create_group(mouse_kobj, &attr_group)) {
        printk(KERN_ERR "Не удалось создать sysfs атрибуты\n");
        kobject_put(mouse_kobj);
        unregister_chrdev(major_number, DEVICE_NAME);
        ring_buffer_free();
        return -ENOMEM;
    }

    printk(KERN_INFO "Драйвер мыши инициализирован с номером %d\n", major_number);
    printk(KERN_INFO "Создайте устройство командой: 'sudo mknod /dev/%s c %d 0'\n", DEVICE_NAME, major_number);
    printk(KERN_INFO "Установите права: 'sudo chmod <major_number> /dev/%s'\n", DEVICE_NAME);
    printk(KERN_INFO "Чтобы изменить режим отображения, используйте: 'echo [0-2] > /sys/mouselog/display_mode'\n");

    return 0;
}

// Функция выхода модуля
static void __exit mouse_driv_exit(void)
{
    // Удаляем sysfs атрибуты
    kobject_put(mouse_kobj);

    // Удаляем символьное устройство
    unregister_chrdev(major_number, DEVICE_NAME);

    // Освобождаем буфер
    ring_buffer_free();

    printk(KERN_INFO "Драйвер мыши удален\n");
}

// Регистрация функций инициализации и выхода
module_init(mouse_driv_init);
module_exit(mouse_driv_exit);
