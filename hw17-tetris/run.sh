#!/bin/bash

# Более сложное меню (требует dialog или whiptail)
# Установите: sudo apt install dialog

show_dialog_menu() {
    dialog --clear --backtitle "Игра Тетрис" \
           --title "🌟 ВЫБЕРИТЕ ДЕЙСТВИЕ 🌟" \
           --menu "Выберите опцию:" 15 45 5 \
           1 "⚡ Запустить игру" \
           2 "💻 Управление" \
           3 "📊 Показать таблицу победителей" \
           4 "❌ Удалить таблицу победителей" \
           5 "🚪 Выход" \
           2> /tmp/menu.choice
    
    choice=$(cat /tmp/menu.choice)
    
    case $choice in
        1) 
            clear
            ./hw17-tetris
            ;;
        2) 
            dialog --infobox "W-Вращение\nA-Влево\nS-Ускорение\nD-Вправо" 6 40
            sleep 2
            clear
            ;;
        3)
            if test -f scoreboard.txt; then
                dialog --textbox scoreboard.txt 10 40
            else
                dialog --infobox "Таблица победителей пуста!" 10 40
                sleep 2
                clear
            fi
            ;;
        4)
            if test -f scoreboard.txt; then
                rm scoreboard.txt
            fi
            dialog --infobox "Таблица победителей удалена!" 10 40
            sleep 2
            clear
            ;;
        5)
            dialog --infobox "До свидания!" 3 20
            sleep 2
            clear
            exit 0
            ;;
    esac
}

# Запуск меню
while true; do
    show_dialog_menu
done
