#include "displayapp/screens/Notifications.h"
#include "displayapp/DisplayApp.h"
#include "components/ble/MusicService.h"
#include "components/ble/AlertNotificationService.h"
#include "displayapp/screens/Symbols.h"
#include <algorithm>
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;
extern lv_font_t jetbrains_mono_extrabold_compressed;
extern lv_font_t inter_semibold_20;

namespace {
  struct CategoryStyle {
    const char* icon;
    lv_color_t color;
  };

  CategoryStyle StyleFor(Pinetime::Controllers::NotificationManager::Categories category) {
    using Category = Pinetime::Controllers::NotificationManager::Categories;
    switch (category) {
      case Category::Sms:
      case Category::InstantMessage:
        return {Symbols::comment, Colors::green};
      case Category::Email:
        return {Symbols::envelope, Colors::blue};
      case Category::IncomingCall:
      case Category::MissedCall:
      case Category::VoiceMail:
        return {Symbols::phone, Colors::green};
      case Category::Schedule:
        return {Symbols::calendar, Colors::red};
      case Category::News:
        return {Symbols::newspaper, Colors::pink};
      case Category::HighProriotyAlert:
        return {Symbols::exclamation, Colors::red};
      default:
        return {Symbols::bell, Colors::gray};
    }
  }
}

Notifications::Notifications(DisplayApp* app,
                             Pinetime::Controllers::NotificationManager& notificationManager,
                             Pinetime::Controllers::AlertNotificationService& alertNotificationService,
                             Pinetime::Controllers::MotorController& motorController,
                             System::SystemTask& systemTask,
                             Modes mode)
  : app {app},
    notificationManager {notificationManager},
    alertNotificationService {alertNotificationService},
    motorController {motorController},
    wakeLock(systemTask),
    mode {mode} {

  notificationManager.ClearNewNotificationFlag();
  auto notification = notificationManager.GetLastNotification();
  if (notification.valid) {
    currentId = notification.id;
    currentItem = std::make_unique<NotificationItem>(notification.Title(),
                                                     notification.Message(),
                                                     1,
                                                     notification.category,
                                                     notificationManager.NbNotifications(),
                                                     alertNotificationService,
                                                     motorController);
    validDisplay = true;
  } else {
    currentItem = std::make_unique<NotificationItem>(alertNotificationService, motorController);
    validDisplay = false;
  }
  if (mode == Modes::Preview) {
    wakeLock.Lock();
    if (notification.category == Controllers::NotificationManager::Categories::IncomingCall) {
      motorController.StartRinging();
    } else {
      motorController.RunForDuration(35);
    }

    timeoutLine = lv_line_create(lv_scr_act(), nullptr);

    lv_obj_set_style_local_line_width(timeoutLine, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, 3);
    lv_obj_set_style_local_line_color(timeoutLine, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, Colors::secondaryLabel);
    lv_obj_set_style_local_line_rounded(timeoutLine, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, true);

    lv_line_set_points(timeoutLine, timeoutLinePoints, 2);
    timeoutTickCountStart = xTaskGetTickCount();
  }

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
}

Notifications::~Notifications() {
  lv_task_del(taskRefresh);
  // make sure we stop any vibrations before exiting
  motorController.StopRinging();
  lv_obj_clean(lv_scr_act());
}

void Notifications::Refresh() {
  if (mode == Modes::Preview && timeoutLine != nullptr) {
    TickType_t tick = xTaskGetTickCount();
    int32_t pos = LV_HOR_RES - ((tick - timeoutTickCountStart) / (timeoutLength / LV_HOR_RES));
    if (pos <= 0) {
      running = false;
    } else {
      timeoutLinePoints[1].x = pos;
      lv_line_set_points(timeoutLine, timeoutLinePoints, 2);
    }

  } else if (mode == Modes::Preview && dismissingNotification) {
    running = false;

  } else if (dismissingNotification) {
    dismissingNotification = false;
    auto notification = notificationManager.Get(currentId);
    if (!notification.valid) {
      notification = notificationManager.GetLastNotification();
    }
    currentId = notification.id;

    if (!notification.valid) {
      validDisplay = false;
    }

    currentItem.reset(nullptr);
    if (afterDismissNextMessageFromAbove) {
      app->SetFullRefresh(DisplayApp::FullRefreshDirections::Down);
    } else {
      app->SetFullRefresh(DisplayApp::FullRefreshDirections::Up);
    }

    if (validDisplay) {
      Controllers::NotificationManager::Notification::Idx currentIdx = notificationManager.IndexOf(currentId);
      currentItem = std::make_unique<NotificationItem>(notification.Title(),
                                                       notification.Message(),
                                                       currentIdx + 1,
                                                       notification.category,
                                                       notificationManager.NbNotifications(),
                                                       alertNotificationService,
                                                       motorController);
    } else {
      running = false;
    }
  }

  running = running && currentItem->IsRunning();
}

void Notifications::OnPreviewInteraction() {
  wakeLock.Release();
  motorController.StopRinging();
  mode = Modes::Normal;
  if (timeoutLine != nullptr) {
    lv_obj_del(timeoutLine);
    timeoutLine = nullptr;
  }
}

void Notifications::DismissToBlack() {
  currentItem.reset(nullptr);
  app->SetFullRefresh(DisplayApp::FullRefreshDirections::RightAnim);
  // create black transition screen to let the notification dismiss to blackness
  lv_obj_t* blackBox = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(blackBox, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_style_local_bg_color(blackBox, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  dismissingNotification = true;
}

void Notifications::OnPreviewDismiss() {
  notificationManager.Dismiss(currentId);
  if (timeoutLine != nullptr) {
    lv_obj_del(timeoutLine);
    timeoutLine = nullptr;
  }
  DismissToBlack();
}

bool Notifications::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  if (mode == Modes::Preview) {
    if (event == TouchEvents::Tap || event == TouchEvents::SwipeDown) {
      OnPreviewInteraction();
    } else if (event == Pinetime::Applications::TouchEvents::SwipeRight) {
      OnPreviewDismiss();
      return true;
    } else {
      return false;
    }
  }

  switch (event) {
    case Pinetime::Applications::TouchEvents::SwipeRight:
      if (validDisplay) {
        auto previousMessage = notificationManager.GetPrevious(currentId);
        auto nextMessage = notificationManager.GetNext(currentId);
        afterDismissNextMessageFromAbove = previousMessage.valid;
        notificationManager.Dismiss(currentId);
        if (previousMessage.valid) {
          currentId = previousMessage.id;
        } else if (nextMessage.valid) {
          currentId = nextMessage.id;
        } else {
          // don't update id, notification manager will try to fetch
          // but not find it. Refresh will try to load latest message
          // or dismiss to previous screen
        }
        DismissToBlack();
        return true;
      }
      return false;
    case Pinetime::Applications::TouchEvents::SwipeDown: {
      Controllers::NotificationManager::Notification previousNotification;
      if (validDisplay) {
        previousNotification = notificationManager.GetPrevious(currentId);
      } else {
        previousNotification = notificationManager.GetLastNotification();
      }

      if (!previousNotification.valid) {
        return true;
      }

      currentId = previousNotification.id;
      Controllers::NotificationManager::Notification::Idx currentIdx = notificationManager.IndexOf(currentId);
      validDisplay = true;
      currentItem.reset(nullptr);
      app->SetFullRefresh(DisplayApp::FullRefreshDirections::Down);
      currentItem = std::make_unique<NotificationItem>(previousNotification.Title(),
                                                       previousNotification.Message(),
                                                       currentIdx + 1,
                                                       previousNotification.category,
                                                       notificationManager.NbNotifications(),
                                                       alertNotificationService,
                                                       motorController);
    }
      return true;
    case Pinetime::Applications::TouchEvents::SwipeUp: {
      Controllers::NotificationManager::Notification nextNotification;
      if (validDisplay) {
        nextNotification = notificationManager.GetNext(currentId);
      } else {
        nextNotification = notificationManager.GetLastNotification();
      }

      if (!nextNotification.valid) {
        running = false;
        return false;
      }

      currentId = nextNotification.id;
      Controllers::NotificationManager::Notification::Idx currentIdx = notificationManager.IndexOf(currentId);
      validDisplay = true;
      currentItem.reset(nullptr);
      app->SetFullRefresh(DisplayApp::FullRefreshDirections::Up);
      currentItem = std::make_unique<NotificationItem>(nextNotification.Title(),
                                                       nextNotification.Message(),
                                                       currentIdx + 1,
                                                       nextNotification.category,
                                                       notificationManager.NbNotifications(),
                                                       alertNotificationService,
                                                       motorController);
    }
      return true;
    default:
      return false;
  }
}

namespace {
  void CallEventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* item = static_cast<Notifications::NotificationItem*>(obj->user_data);
    item->OnCallButtonEvent(obj, event);
  }
}

Notifications::NotificationItem::NotificationItem(Pinetime::Controllers::AlertNotificationService& alertNotificationService,
                                                  Pinetime::Controllers::MotorController& motorController)
  : NotificationItem("Notifications",
                     "No notifications to display",
                     0,
                     Controllers::NotificationManager::Categories::Unknown,
                     0,
                     alertNotificationService,
                     motorController) {
}

Notifications::NotificationItem::NotificationItem(const char* title,
                                                  const char* msg,
                                                  uint8_t notifNr,
                                                  Controllers::NotificationManager::Categories category,
                                                  uint8_t notifNb,
                                                  Pinetime::Controllers::AlertNotificationService& alertNotificationService,
                                                  Pinetime::Controllers::MotorController& motorController)
  : alertNotificationService {alertNotificationService}, motorController {motorController} {
  using Category = Controllers::NotificationManager::Categories;

  container = lv_cont_create(lv_scr_act(), nullptr);
  lv_obj_set_size(container, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_style_local_bg_color(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_set_style_local_radius(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_pad_all(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_pad_inner(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_border_width(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

  if (notifNb == 0) {
    lv_obj_t* empty = lv_label_create(container, nullptr);
    lv_obj_set_style_local_text_color(empty, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::secondaryLabel);
    lv_label_set_text_static(empty, "No Notifications");
    lv_obj_align(empty, nullptr, LV_ALIGN_CENTER, 0, 0);
    subject_container = nullptr;
    return;
  }

  const CategoryStyle style = StyleFor(category);

  lv_obj_t* appIcon = lv_obj_create(container, nullptr);
  lv_obj_set_size(appIcon, 30, 30);
  lv_obj_set_pos(appIcon, 10, 10);
  lv_obj_set_style_local_radius(appIcon, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
  lv_obj_set_style_local_bg_color(appIcon, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, style.color);
  lv_obj_t* appGlyph = lv_label_create(appIcon, nullptr);
  lv_label_set_text_static(appGlyph, style.icon);
  lv_obj_align(appGlyph, nullptr, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t* alert_count = lv_label_create(container, nullptr);
  lv_obj_set_style_local_text_color(alert_count, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::secondaryLabel);
  lv_label_set_text_fmt(alert_count, "%i/%i", notifNr, notifNb);
  lv_obj_align(alert_count, nullptr, LV_ALIGN_IN_TOP_RIGHT, -10, 14);

  lv_obj_t* alert_type = lv_label_create(container, nullptr);
  if (title == nullptr) {
    lv_label_set_text_static(alert_type, "Notification");
  } else {
    // copy title to label and replace newlines with spaces
    lv_label_set_text(alert_type, title);
    char* pchar = strchr(lv_label_get_text(alert_type), '\n');
    while (pchar != nullptr) {
      *pchar = ' ';
      pchar = strchr(pchar + 1, '\n');
    }
    lv_label_refr_text(alert_type);
  }
  lv_label_set_long_mode(alert_type, LV_LABEL_LONG_SROLL_CIRC);
  lv_obj_set_width(alert_type, 140);
  lv_obj_set_pos(alert_type, 48, 14);

  subject_container = lv_cont_create(container, nullptr);
  lv_obj_set_style_local_bg_color(subject_container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, Colors::platter);
  lv_obj_set_style_local_radius(subject_container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 20);
  lv_obj_set_style_local_pad_all(subject_container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 14);
  lv_obj_set_style_local_pad_inner(subject_container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_border_width(subject_container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_pos(subject_container, 4, 50);
  lv_obj_set_size(subject_container, LV_HOR_RES - 8, LV_VER_RES - 54);
  lv_cont_set_layout(subject_container, LV_LAYOUT_COLUMN_LEFT);
  lv_cont_set_fit(subject_container, LV_FIT_NONE);

  lv_obj_t* alert_subject = lv_label_create(subject_container, nullptr);
  lv_label_set_long_mode(alert_subject, LV_LABEL_LONG_BREAK);
  lv_obj_set_width(alert_subject, LV_HOR_RES - 36);

  switch (category) {
    default:
      lv_label_set_text(alert_subject, msg);
      break;
    case Category::IncomingCall: {
      lv_obj_set_height(subject_container, 96);
      lv_obj_set_style_local_text_color(alert_subject, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::secondaryLabel);
      lv_label_set_text_static(alert_subject, "Incoming call");

      lv_obj_t* alert_caller = lv_label_create(subject_container, nullptr);
      lv_label_set_long_mode(alert_caller, LV_LABEL_LONG_BREAK);
      lv_obj_set_width(alert_caller, LV_HOR_RES - 36);
      lv_label_set_text(alert_caller, msg);

      auto callButton = [this, container = container](lv_obj_t*& label, const char* icon, lv_color_t color, lv_align_t align) {
        lv_obj_t* button = lv_btn_create(container, nullptr);
        button->user_data = this;
        lv_obj_set_event_cb(button, CallEventHandler);
        lv_obj_set_size(button, 68, 68);
        lv_obj_align(button, nullptr, align, 0, -8);
        lv_obj_set_style_local_bg_color(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, color);
        label = lv_label_create(button, nullptr);
        lv_label_set_text_static(label, icon);
        return button;
      };
      bt_reject = callButton(label_reject, Symbols::phoneSlash, Colors::red, LV_ALIGN_IN_BOTTOM_LEFT);
      lv_obj_set_x(bt_reject, 10);
      bt_mute = callButton(label_mute, Symbols::volumMute, Colors::button, LV_ALIGN_IN_BOTTOM_MID);
      bt_accept = callButton(label_accept, Symbols::phone, Colors::green, LV_ALIGN_IN_BOTTOM_RIGHT);
      lv_obj_set_x(bt_accept, LV_HOR_RES - 78);
    } break;
  }
}

void Notifications::NotificationItem::OnCallButtonEvent(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) {
    return;
  }

  motorController.StopRinging();

  if (obj == bt_accept) {
    alertNotificationService.AcceptIncomingCall();
  } else if (obj == bt_reject) {
    alertNotificationService.RejectIncomingCall();
  } else if (obj == bt_mute) {
    alertNotificationService.MuteIncomingCall();
  }

  running = false;
}

Notifications::NotificationItem::~NotificationItem() {
  lv_obj_clean(lv_scr_act());
}
